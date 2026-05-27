
#include <unistd.h>
#include <filesystem>

#include <string>

#include "support.h"
#include <fstream>
#include "trelloapi.h"

LOG_TAG(TrelloApi);
trello_api::trello_api(const std::string api_key, const std::string user_token, const std::string prefix)
    : api_key_(std::move(api_key)), user_token_(std::move(user_token))
{
    apiprefix_ = prefix;
    cache_path_ = "cache";
    std::filesystem::create_directory(cache_path_);
}

trello_result<std::string> trello_api::delete_all_cards_from_archive(const std::string list_id)
{
    std::string res;

    return res;
}

int trello_api::add_img_to_card(const std::string card_id, const std::string imgPath)
{
    c_out << "\nAdding image " << imgPath << " to card " << card_id << std::endl;
    std::string reqURL = apiprefix_ + "cards/" + card_id + "/attachments?";
    reqURL.append("key=").append(api_key_).append("&token=").append(user_token_);

    std::string contents;
    std::ifstream in(imgPath, std::ios::in | std::ios::binary);
/*  std::cout << "Seeking image contents..." << std::endl;          */
    if (in)
    {
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
    }

    CURLcode res;

    struct curl_httppost *formpost = nullptr;
    struct curl_httppost *lastptr = nullptr;
    struct curl_slist *headerlist = nullptr;
    static const char buf[] =  "Expect:";

    curl_global_init(CURL_GLOBAL_ALL);

    // set up the header
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "cache-control:",
                 CURLFORM_COPYCONTENTS, "no-cache",
                 CURLFORM_END);

    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "content-type:",
                 CURLFORM_COPYCONTENTS, "multipart/form-data",
                 CURLFORM_END);

    const std::string img_file_name = std::filesystem::path(imgPath).filename();

    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "file",  // in this case wanted file-Tag
                 CURLFORM_BUFFER, img_file_name.c_str(),
                 CURLFORM_BUFFERPTR, contents.data(),
                 CURLFORM_BUFFERLENGTH, contents.size(),
                 CURLFORM_END);

/*  std::cout << "addImgToCard: Init curl..." << std::endl; */
    CURL* curl = curl_easy_init();

    headerlist = curl_slist_append(headerlist, buf);
    if (curl) {

        curl_easy_setopt(curl, CURLOPT_URL, reqURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

/*      std::cout << "addImgToCard: curl perform..." << std::endl;    */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
        {
            c_out << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return 7;
        }

        std::cout << std::endl;
        curl_easy_cleanup(curl);        
        curl_formfree(formpost);
        curl_slist_free_all(headerlist);
    }
    std::cout << std::endl;
    return 0;
}

size_t trello_api::curl_write_callback(char* buff_ptr, size_t sz, size_t cnt_memb, void* userdata)
{
    /* userdata points to the TrelloApi instance */
    auto res = static_cast<trello_api *>(userdata);

    const auto str = (std::string *)res;
    str->append(buff_ptr, 0, sz * cnt_memb);
    return sz * cnt_memb;
}

trello_result<jira_card> trello_api::get_card(const std::string &card_id)
{
    jira_card trello_card;

    std::cout << return_current_time_and_date() << ": Запрашиваем карточку " + card_id << " ";
    CURL *curl_req = curl_easy_init();
    if (!curl_req) {
        return trello_error::unknown;
    }

    auto json_resp = new std::string;

    std::string full_url = apiprefix_;
    full_url.append("cards/").append(card_id).append("?key=").append(api_key_).append("&token=").append(user_token_);
    curl_easy_setopt(curl_req, CURLOPT_URL, full_url.c_str());

    /* указатель на функцию, которая будет сохранять получаемые данные в наш буфер */
    curl_easy_setopt(curl_req, CURLOPT_WRITEFUNCTION, curl_write_callback);
    /* 22.8.25 curl_easy_setopt(curl_req, CURLOPT_WRITEFUNCTION, curlWriteDataString); */

    curl_easy_setopt(curl_req, CURLOPT_WRITEDATA, json_resp);
    curl_easy_setopt(curl_req, CURLOPT_HTTPGET, 1);

/*  c_out << "Performing easy on curl..." << std::endl; */
    const int res = curl_easy_perform(curl_req);

    long status_code = 0;
    if (res == CURLE_OK)
    {
/*      c_out << "res = CURLE_OK, so => curl_easy_getinfo(curl_req, CURLINFO_RESPONSE_CODE, 0)" << std::endl;   */
        curl_easy_getinfo(curl_req, CURLINFO_RESPONSE_CODE, &status_code);
    }
    else {
        c_out << "curl_easy_perform's result wasn't OK, code: " + itoa(res) << std::endl;
    }

    if (status_code > 300)
        c_out << &"HTTP Status Code: " [ status_code] << std::endl;
    if (status_code == 404)
    {
        c_out << "Карточка не существует (404), необходимо её удалить из БД, jiraError::NotFound_404, exiting..." << std::endl;
        return trello_error::not_found_404;
    }

    if (status_code < 200 || status_code >= 300) {
        c_out << "Request failed " + itoa(res) + " status code " + itoa(status_code) << std::endl;
        return trello_error::request_failed;
    }

    boost::json::error_code ec;
    const boost::json::value card_obj_ptr = boost::json::parse(json_resp->c_str(), ec);
    if (ec.value() != 0) {
        c_out << "GetCard failed to parse request's JSON" << std::endl;
        return trello_error::request_invalid;
    }

    try {
        std::string jira_card_name = value_to<std::string>(card_obj_ptr.at("name"));
//        std::string jiraCardName = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(card_obj_ptr, "name"));

        const std::string str_escaped = std::regex_replace(jira_card_name, std::regex("[\r\t\n]+" ), "");
        trello_card.name = truncate_msg_from_user(str_escaped, 55);

        std::cout << "(" << trello_card.name << ")" << std::endl;
    } catch (std::exception e) {
        std::cout << e.what() << std::endl;
    }

    const auto is_closed = value_to<bool>(card_obj_ptr.at("closed"));
//    const auto jsonStatus = cJSON_GetObjectItemCaseSensitive(card_obj_ptr, "closed");
//    const bool is_closed = jsonStatus->valueint;
//    const bool is_closed = value_to<int>(card_obj_ptr.at("closed"));
    if (is_closed)
    {
        trello_card.is_closed = true;
        std::cout << "                                    "; // << std::endl;
        c_out << "Карточка " + card_id + " удалена" << std::endl;
    }
    else
    {
        trello_card.is_closed = false;
        /* c_out("Is closed? " + is_closed); */
    }

    const auto jira_list_id = value_to<std::string>(card_obj_ptr.at("idList"));
//    const std::string jiraListID = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(card_obj_ptr, "idList"));
    trello_card.id_list = jira_list_id;

    trello_card.id = card_id;    

    curl_easy_cleanup(curl_req);

    if(json_resp)
        delete json_resp;

    return trello_card;
}

int trello_api::change_card_color(const std::string& card_id, const std::string& color)
{
    auto json_body = new std::string;
    c_out << "Updating color of card " + card_id + " to " + color << std::endl;

    CURL *curl_req = curl_easy_init();
    if (!curl_req) {
        return 1;
    }
    curl_easy_setopt(curl_req, CURLOPT_CUSTOMREQUEST, "PUT");

    // Header
    curl_slist *post_header_list = nullptr;
    post_header_list = curl_slist_append(post_header_list, HEADER_CONTENT_TYPE_APP_JSON);

    curl_easy_setopt(curl_req, CURLOPT_HTTPHEADER, post_header_list);
    std::string str_json = " { \"key\": \"" + api_key_ + "\", \"token\": \"" + user_token_ + "\" ,";
    str_json.append("\"cover\": {\"color\": \"" + color + "\", \"size\": \"full\", \"brightness\": \"light\" } } ");

    std::string full_url = apiprefix_;
    full_url.append("cards/").append(card_id);

    curl_easy_setopt(curl_req, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl_req, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl_req, CURLOPT_WRITEDATA, json_body);
    curl_easy_setopt(curl_req, CURLOPT_POSTFIELDS, str_json.c_str());

    const int res = curl_easy_perform(curl_req);

    long status_code = 0;
    if (res == CURLE_OK)
    {
        curl_easy_getinfo(curl_req, CURLINFO_RESPONSE_CODE, &status_code);
    }
    else
    {
        std::cout << "curl_easy_perform(curl_req)'s result wasn't OK, code: " << res << std::endl;
    }

    std::cout << "HTTP Status code: " << status_code << std::endl;
    if (status_code < 200 || status_code >= 300)
    {
        std::cout << "Request failed " << res << " status code " << status_code << std::endl;
        return 2;
    }

    boost::json::error_code ec;
    const boost::json::value response_json = boost::json::parse(json_body->c_str(), ec);
    if (ec.value() != 0) {
        c_out << "ChangeCardColor failed to parse request's JSON" << std::endl;
        return 3;
    }

    curl_slist_free_all(post_header_list);
    curl_easy_cleanup(curl_req);

    colour_out("Color of the card successfully updated");

    if(json_body)
        delete json_body;
    return 0;
}

int trello_api::archiving_card(const std::string& card_id)
{
    std::string json_body;
    c_out << "Archiving Trello card " << card_id << std::endl;

    CURL *curl_req = curl_easy_init();
    if (!curl_req) {
        return 1;
    }
    curl_easy_setopt(curl_req, CURLOPT_CUSTOMREQUEST, "PUT");

    // Header
    curl_slist *post_header_list = nullptr;
    post_header_list = curl_slist_append(post_header_list, HEADER_CONTENT_TYPE_APP_JSON);

    curl_easy_setopt(curl_req, CURLOPT_HTTPHEADER, post_header_list);
    std::string strJSON = " { \"key\": \"" + api_key_ + "\", \"token\": \"" + user_token_ + "\" , \"closed\": \"true\" } ";


    std::string full_url = apiprefix_;
    full_url.append("cards/").append(card_id);

    curl_easy_setopt(curl_req, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl_req, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl_req, CURLOPT_WRITEDATA, &json_body);
    curl_easy_setopt(curl_req, CURLOPT_POSTFIELDS, strJSON.c_str());

    const int res = curl_easy_perform(curl_req);

    long status_code = 0;
    if (res == CURLE_OK)
    {
        curl_easy_getinfo(curl_req, CURLINFO_RESPONSE_CODE, &status_code);
    }
    else
    {
        std::cout << "curl_easy_perform()'s result was not OK, code: " << res << std::endl;
    }

    c_out << "HTTP status Code: " << itoa(status_code) << std::endl;
    if (status_code < 200 || status_code >= 300)
    {
        std::cout << "Request failed " << res << " status code " << status_code << std::endl;
        return 2;
    }

    boost::json::error_code ec;
    const boost::json::value response_json = boost::json::parse(json_body.c_str(), ec);
    if (ec.value() != 0) {
        c_out << "ArchivingCard failed to parse request's JSON" << std::endl;
        return 3;
    }

    curl_slist_free_all(post_header_list);
    curl_easy_cleanup(curl_req);

    colour_out("The card was successfully archived");

    return 0;
}

trello_result<std::string> trello_api::create_new_card(const std::string& list_id, std::string card_name, std::string card_description, std::string** created_card_id)
{
    auto json_resp = new std::string;
    c_out << "Creating new Trello card:" <<  std::endl;

    CURL *curl_req = curl_easy_init();
    if (!curl_req) {
        return trello_error::unknown;
    }
    curl_easy_setopt(curl_req, CURLOPT_HTTPPOST, 1);

    // Header
    curl_slist *post_header_list = nullptr;
    post_header_list = curl_slist_append(post_header_list, HEADER_CONTENT_TYPE_APP_JSON);

    curl_easy_setopt(curl_req, CURLOPT_HTTPHEADER, post_header_list);

    /* rib_26.2.25 Чистим все ENTER'ы, заменяем слеши, кавычки, из JSON'а, иначе будет error parsing body: */

    card_description = escape_string(card_description);
    boost::replace_all(card_description, "{", "\\\\{");
    card_name = escape_string(card_name);
    boost::replace_all(card_name, "{", "\\\\{");

    std::string str_json = " { \"name\":\"";
    str_json.append(card_name).append("\", \"desc\": \"").append(card_description).append("\" } ");

    std::string full_url = apiprefix_;
    full_url.append("cards?idList=").append(list_id).append("&key=").append(api_key_).append("&token=").append(user_token_);

    c_out << "\n" << str_json << std::endl;

    curl_easy_setopt(curl_req, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl_req, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl_req, CURLOPT_WRITEDATA, json_resp);
    curl_easy_setopt(curl_req, CURLOPT_POSTFIELDS, str_json.c_str());

    const int res = curl_easy_perform(curl_req);

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
        return trello_error::request_failed;
    }

    boost::json::error_code ec;
    const boost::json::value card_id_ptr = boost::json::parse(json_resp->c_str(), ec);
    if (ec.value() != 0) {
        c_out << "CreateNewCard failed to parse request's JSON" << std::endl;
        return trello_error::request_invalid;
    }

    std::string card_id = value_to<std::string>(card_id_ptr.at("id"));
//  std::string card_id = card_id_ptr->child->valuestring;

    curl_slist_free_all(post_header_list);
    curl_easy_cleanup(curl_req);

    std::cout << json_resp->c_str() << std::endl;


    /* Указатель будет удален после выхода из ф-ции и использования */
    *created_card_id = new std::string(card_id);

    if(json_resp)
        delete json_resp;

    return static_cast<trello_result<std::string>>(card_id);
}

bool trello_card::has_label(char const *label) const
{
    for (auto card_label : labels_)
    {
        if (card_label == label) {
            return true;
        }
    }

    return false;
}

bool trello_card::is_match(const std::string &field, const std::string &value) {
    if (value.empty()) {
        return true;
    }

    std::istringstream value_stream(value);
    std::string word;

    while (value_stream >> word) {
        if (field.find(word) == std::string::npos) {
            return false;
        }
    }

    return true;
}

std::string trello_api::get_cache_key(const std::string &url) { return sha1(url); }

trello_result<std::string> trello_api::get_file(const std::string &url, bool force) { return get_file(url, ".cache", force); }

trello_result<std::string> trello_api::get_file(const std::string &url, const std::string &extension, bool force) {
    const auto cache_key = get_cache_key(url);
    const auto file_name = strformat("%s/%s%s", cache_path_.c_str(), cache_key.c_str(), extension.c_str());
    if (std::filesystem::exists(file_name)) {
        if (force) {
            // Force the cache to be invalidated.
            std::filesystem::remove(file_name);
        } else {
            return file_name;
        }
    }

    const auto curl = curl_easy_init();
    if (!curl) {
        return trello_error::unknown;
    }

    const auto authorization_header = strformat(R"(Authorization: OAuth oauth_consumer_key="%s", oauth_token="%s")",
                                          api_key_.c_str(), user_token_.c_str());

    auto headers = curl_slist_append(nullptr, authorization_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    auto tmp_file_name = file_name + ".tmp";

    std::ofstream of_file(tmp_file_name, std::ios::binary);

    auto full_url = url;
    if (full_url.find("://") == std::string::npos) {
        full_url = strformat("https://api.trello.com/%s", full_url.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(
      curl, CURLOPT_WRITEFUNCTION, (curl_write_cb_t *)[](void *contents, size_t size, size_t nmemb, void *userp) {
            auto *pfile = (std::ofstream *)userp;
            pfile->write((char *)contents, size * nmemb);
            return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &of_file);
    curl_easy_setopt(curl, CURLOPT_CAINFO, getenv("CURL_CA_BUNDLE"));

    LOGI(TAG, "Requesting %s", full_url.c_str());

    CURLcode res;

    for (auto i = 0; i < 10; i++) {
        res = curl_easy_perform(curl);

        if (res == CURLE_COULDNT_RESOLVE_HOST) {
            sleep(5);

            LOGW(TAG, "Retrying...");
        } else {
            break;
        }
    }

    of_file.close();

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    long status_code = 0;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    }

    if (status_code < 200 || status_code >= 300) {
        std::filesystem::remove(tmp_file_name);

        LOGE(TAG, "Request failed %d status code %d", (int)res, (int)status_code);

        return trello_error::request_failed;
    }

    std::filesystem::remove(file_name);
    std::filesystem::rename(tmp_file_name, file_name);

    return file_name;
}

trello_result<std::string> trello_api::get_image_file(const std::string &url, std::optional<int> width, std::optional<int> height,
                                               bool force) {
    const auto curl = curl_session(curl_easy_init());
    if (!*curl) {
        return trello_error::unknown;
    }

    auto url_encoded = curl_data(curl_easy_escape(*curl, url.c_str(), 0));
    if (!*url_encoded) {
        return trello_error::unknown;
    }

    std::string new_url = *url_encoded;

    if (width.has_value()) {
        new_url += strformat("&width=%d", width.value());
    }
    if (height.has_value()) {
        new_url += strformat("&height=%d", height.value());
    }

    return get_file(new_url, ".bin", force);
}
