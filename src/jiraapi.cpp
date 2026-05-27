#include "jiraapi.h"

#ifdef LISTCHECKER
/* listchecker compile with Boost:json: */
#include <boost/json/src.hpp>
#endif

jira_api::jira_api(const std::string& token, const std::string& base_url, const TgBot::Api &api, const std::string& support_tg_id, const QSqlDatabase& pgsql_db, const std::string& mail_script_path): m_pg_database_(const_cast<QSqlDatabase&>(pgsql_db))
{
    m_pg_database_ = pgsql_db;
    m_jira_token_ = token;
    m_base_url_ = QString::fromStdString(base_url);
    tg_bot_api_ = &api;
    m_support_tg_id_ = atol(support_tg_id.c_str());
    m_mail_sender_path_ = mail_script_path;
}

jira_api::~jira_api()
{
    curl_global_cleanup();
}

size_t jira_api::curl_write_callback(const char* buff_ptr, const size_t sz, const size_t cnt_memb, void* userdata)
{
    /* userdata points to the TrelloApi instance */
    const auto res = static_cast<trello_api *>(userdata);

    const auto str = (std::string *)res;
    str->append(buff_ptr, 0, sz * cnt_memb);
    return sz * cnt_memb;
}

int jira_api::create_task(user_dispatcher* ud, const std::string& project_name)
{
    auto json_resp = new std::string;
    c_out << "Creating new Jira task:" <<  std::endl;

    CURL *curl_req = curl_easy_init();
    if (!curl_req) {
        return 1;
    }
    curl_easy_setopt(curl_req, CURLOPT_HTTPPOST, 1);

    const std::string bearer_token = "Authorization: Bearer " + m_jira_token_;
    // Header
    curl_slist *post_header_list = nullptr;
    post_header_list = curl_slist_append(post_header_list, HEADER_CONTENT_TYPE_APP_JSON);
    post_header_list = curl_slist_append(post_header_list, "Accept: application/json");
    post_header_list = curl_slist_append(post_header_list, bearer_token.c_str());

    curl_easy_setopt(curl_req, CURLOPT_HTTPHEADER, post_header_list);

    /* Заголовок Summary (+проект) не должен быть длиннее 255 после экранирования слэшами */
    std::string s_subj = truncate_msg_from_user(ud->jira_task.subj, 100);
    std::string s_project = truncate_msg_from_user(ud->jira_task.project, 100);

    const std::string str_json = R"({ "fields": { "project" : { "key": ")" + project_name + R"(" }, "issuetype" : { "name": "Задача" },
                                     "summary" : ")" + s_subj + "; Проект: " + s_project + /* Summary - это заголовок */
                                R"(", "labels" : [")" + "Секция:" + ud->jira_task.section + "\", \"" + "https://t.me/" + ud->jira_task.consumer_tg_username_link + R"("],
                                      "description" : ")" + ud->jira_task.description +
                                      "\\n\\n--   \\nC уважением,\\n" + ud->jira_task.consumer_name + "\\n" + "Telegram: https://t.me/" + ud->jira_task.consumer_tg_username_link + "\"}}";

    std::string full_url = m_base_url_.toStdString();
    full_url.append("issue");

    c_out << "JSON's content:\n" << str_json << std::endl;

    curl_easy_setopt(curl_req, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl_req, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl_req, CURLOPT_WRITEDATA, json_resp);
    curl_easy_setopt(curl_req, CURLOPT_POSTFIELDS, str_json.c_str());

    curl_easy_setopt(curl_req, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_req, CURLOPT_SSL_VERIFYHOST, 0L);
    int res = curl_easy_perform(curl_req);

    long status_code = 0;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl_req, CURLINFO_RESPONSE_CODE, &status_code);
    }
    else {
        std::cout << "curl_easy_perform()'s result was not okay, code: " << res << std::endl;
    }

    c_out << "HTTP status code: " << itoa(status_code) << "\n" << std::endl;

    if (status_code < 200 || status_code >= 300) {

        std::cout << "Request failed " << res << " status code " << status_code << std::endl;
        /* здесь подробный текст ошибки Jira */
        c_out << json_resp->c_str() << std::endl;
        return 2;
    }

    boost::json::error_code ec;
    const boost::json::value json_response = boost::json::parse(json_resp->c_str(), ec);
    if (ec.value() != 0) {
        c_out << "Boost failed to parse request's JSON" << std::endl;
        return 3;
    }

    std::cout << "JSONAnswer:" << json_resp->c_str() << std::endl;
    std::string str_key = "";
    int created_issue = 0;
    try
    {
        str_key = value_to<std::string>(json_response.at("key"));
        created_issue = atol(str_key.substr(str_key.find("-") + 1).c_str());
    }
    catch(const std::exception& e)
    {
        c_out << "Boost failed to parse JSON request" << std::endl;
        std::cerr << e.what() << std::endl;
    }

    curl_slist_free_all(post_header_list);
    curl_easy_cleanup(curl_req);

    std::cout << "JSONAnswer:" << json_resp->c_str() << std::endl;

    if(json_resp)
        delete json_resp;

    if (status_code > 300){

        const QString qErrata = QString::fromUtf8(json_resp->c_str());
        c_out << str_json << std::endl;
        colour_out("Jira's JSON message: " + qErrata.toStdString());
        tg_bot_api_->sendMessage(m_support_tg_id_, qErrata.toStdString());
        return 4;
      }

    return created_issue;
}

bool jira_api::add_attach(const std::string& file_path, const std::string& issue_key)
{
    CURL *curl_req;
    CURLcode res;

    c_out << "Uploading an attachment into Jira's issue " << issue_key <<  std::endl;
    const std::string url = m_base_url_.toStdString() + "issue/" + issue_key + "/attachments";
    const std::string bearer_token = "Authorization: Bearer " + m_jira_token_;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_req = curl_easy_init();

    if(curl_req) {
        curl_easy_setopt(curl_req, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_req, CURLOPT_POST, 1L);

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, bearer_token.c_str());
        headers = curl_slist_append(headers, "X-Atlassian-Token: no-check");
        curl_easy_setopt(curl_req, CURLOPT_HTTPHEADER, headers);

        // Set the file to attach
        curl_httppost* form = nullptr;
        curl_httppost* last = nullptr;
        curl_formadd(&form, &last, CURLFORM_COPYNAME, "file", CURLFORM_FILE, file_path.c_str(), CURLFORM_END);

        // Attach the form data
        curl_easy_setopt(curl_req, CURLOPT_HTTPPOST, form);
        curl_easy_setopt(curl_req, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_req, CURLOPT_SSL_VERIFYHOST, 0L);
/* debug: curl_easy_setopt(curl_req, CURLOPT_VERBOSE, 1L); */

        res = curl_easy_perform(curl_req);

        if(res != CURLE_OK) {
            std::cerr << "cURL request failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }

        curl_slist_free_all(headers);
        curl_formfree(form);
    }

    curl_easy_cleanup(curl_req);
    curl_global_cleanup();

    std::cout << "File attached successfully!" << std::endl;
    return true;
}

/* JIRA_IN_PROGRESS = 4 */
int jira_api::change_task_status(const std::string& issue_key, const int new_status)
{
    auto json_resp = new std::string;

    c_out << "Changing issue' status " <<  std::endl;

    CURL *curl_req = curl_easy_init();
    if (!curl_req) {
        return 1;
    }
    curl_easy_setopt(curl_req, CURLOPT_HTTPPOST, 1);

    const std::string bearer_token = "Authorization: Bearer " + m_jira_token_;
    // Header
    curl_slist *post_header_list = nullptr;
    post_header_list = curl_slist_append(post_header_list, HEADER_CONTENT_TYPE_APP_JSON);
    post_header_list = curl_slist_append(post_header_list, "Accept: application/json");
    post_header_list = curl_slist_append(post_header_list, bearer_token.c_str());

    curl_easy_setopt(curl_req, CURLOPT_HTTPHEADER, post_header_list);

    const std::string str_json = R"({ "transition": { "id": ")" + itoa(new_status) + "\" } } ";

    std::string full_url = m_base_url_.toStdString();
    full_url.append("issue/" + issue_key + "/transitions");

    c_out << "JSON's content:\n" << str_json << std::endl;

    curl_easy_setopt(curl_req, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl_req, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl_req, CURLOPT_WRITEDATA, json_resp);
    curl_easy_setopt(curl_req, CURLOPT_POSTFIELDS, str_json.c_str());

    curl_easy_setopt(curl_req, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_req, CURLOPT_SSL_VERIFYHOST, 0L);
    const int res = curl_easy_perform(curl_req);

    long status_code = 0;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl_req, CURLINFO_RESPONSE_CODE, &status_code);
    }
    else {
        std::cout << "change_task_status - curl_easy_perform()'s result wasn't okay, code: " << res << std::endl;
    }

    c_out << "HTTP status code: " << itoa(status_code) << "\n" << std::endl;

    if (status_code < 200 || status_code >= 300) {

        std::cout << "Request failed " << res << " status code " << status_code << std::endl;
        c_out << json_resp->c_str() << std::endl; /* здесь подробный текст ошибки Jira */
        return 2;
    }

    boost::json::error_code ec;
    const boost::json::value json_response = boost::json::parse(json_resp->c_str(), ec);
    if (ec.value() != 0) {
        c_out << "Boost failed to parse request's JSON" << std::endl;
        return 3;
    }

    std::cout << "JSONAnswer:" << json_resp->c_str() << std::endl;

    if(json_resp)
        delete json_resp;

    return 0;
}

int jira_api::get_task_info(const std::string& issue_key)
{
    const std::string url(m_base_url_.toStdString() + "issue/" + issue_key);
    CURLcode res;
    std::string json_resp;

    c_out << "Getting details of Jira's issue " << issue_key <<  std::endl;
    const std::string bearer_token = "Authorization: Bearer " + m_jira_token_;

    CURL* curl_req = curl_easy_init();
    if(curl_req) {

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, bearer_token.c_str());
        headers = curl_slist_append(headers, "X-Atlassian-Token: no-check");
        curl_easy_setopt(curl_req, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl_req, CURLOPT_URL, url.c_str());

        // Set write callback to capture response
        curl_easy_setopt(curl_req, CURLOPT_WRITEFUNCTION, curl_write_callback);
        curl_easy_setopt(curl_req, CURLOPT_WRITEDATA, &json_resp);
        curl_easy_setopt(curl_req, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_req, CURLOPT_SSL_VERIFYHOST, 0L);

        // Perform the request
        res = curl_easy_perform(curl_req);

        long status_code = 0;
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl_req, CURLINFO_RESPONSE_CODE, &status_code);
        }
        else {
            std::cout << "get_task_info - curl_easy_perform()'s result wasn't okay, code: " << res << std::endl;
        }

        c_out << "get_task_info - HTTP status code: " << itoa(status_code) << std::endl;

        if (status_code < 200 || status_code >= 300) {

            std::cout << "get_task_info - Request failed " << res << " status code " << status_code << std::endl;
            return 2;
        }

        boost::property_tree::ptree pt;
        boost::json::error_code ec;

        if (ec.value() != 0) {
            c_out << "get_task_info: Boost failed to parse request's JSON" << std::endl;
            return 3;
        }

        std::cout << "get_task_info - JSONAnswer:" << json_resp.c_str() << std::endl;

        if (status_code > 300){
            const QString q_errata = QString::fromUtf8(json_resp.c_str());
            colour_out("Jira's JSON message: " + q_errata.toStdString());
            tg_bot_api_->sendMessage(m_support_tg_id_, q_errata.toStdString());
            return 4;
        }

        std::string assignee_email = "";
        try
        {
            std::stringstream ss;
            ss << json_resp;
            boost::property_tree::read_json(ss, pt);
            assignee_email = pt.get_child("fields.assignee").find("emailAddress")->second.get_value("emailAddress");

            if (!assignee_email.empty())
            {
                QSqlQuery pg_query(m_pg_database_);
                pg_query.exec("SELECT id FROM contractors WHERE email = '" + QString::fromStdString(assignee_email).toLower() + "'");
                pg_query.next();
                c_out << pg_query.value(0).toString().toStdString() << std::endl;
                send_email(assignee_email, m_mail_sender_path_, "https://vm-srv051.local:8443/browse/" + issue_key);
            }
        }
        catch(const std::exception& e)
        {
            c_out << "Boost failed to parse JSON request, AssigneeEmail is " << assignee_email << std::endl;
            c_out << e.what() << std::endl;
            std::cerr << e.what() << std::endl;
        }
        // Cleanup
        curl_easy_cleanup(curl_req);
    } else {
        c_out << "getTaskInfo " << issue_key << ": Failed to initialize curl" << std::endl;
    }

    return 0;
}

