#include <sstream>
#include <unicode/locid.h>
#include <unicode/normlzr.h>
#include <openssl/evp.h>
#include <unicode/translit.h>
#include <unicode/unistr.h>
#include <unicode/unorm.h>
#include <iomanip>

#include <tgbot/tgbot.h>
#include "structures.h"
#include "chanserv.h"
#include <dirent.h>

#include "support.h"

using namespace TgBot;

extern "C" size_t curlWriteDataGPT(const char *buff, size_t sz, size_t cnt, void *u)
{
    std::string *str = (std::string *)u;
    str->append(buff, 0, sz * cnt);
    return sz * cnt;
}

int get_proc_id_by_name(const std::string proc_name)
{
    int pid = -1;

    // Open the /proc directory
    DIR *dp = opendir("/proc");
    if (dp != nullptr)
    {
        // Enumerate all entries in directory until process found
        struct dirent *dirp;
        while (pid < 0 && (dirp = readdir(dp)))
        {
            // Skip non-numeric entries
            int id = atoi(dirp->d_name);
            if (id > 0)
            {
                // Read contents of virtual /proc/{pid}/cmdline file
                const std::string cmdPath = std::string("/proc/") + dirp->d_name + "/cmdline";
                std::ifstream cmdFile(cmdPath.c_str());
                std::string cmdLine;
                getline(cmdFile, cmdLine);
                if (!cmdLine.empty())
                {
                    // Keep first cmdline item which contains the program path
                    size_t pos = cmdLine.find('\0');
                    if (pos != std::string::npos)
                        cmdLine = cmdLine.substr(0, pos);
                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');
                    if (pos != std::string::npos)
                        cmdLine = cmdLine.substr(pos + 1);
                    // Compare against requested process name
                    if (proc_name == cmdLine)
                        pid = id;
                }
            }
        }
    }

    closedir(dp);

    return pid;
}

std::string escape_json_entire_text(std::string_view input_text) {
    std::string result;
    result.reserve(input_text.length() * 2); // Heuristic for potential growth

    for (char c : input_text) {
        switch (c) {
            case '"' : result += "\\\""; break; // JSON string delimiters must be escaped
            case '\\': result += "\\\\"; break; // Backslash itself must be escaped
            case '\b': result += "\\b"; break; // Backspace
            case '\f': result += "\\f"; break; // Form feed
            case '\n': result += "\\n"; break; // Newline
            case '\r': result += "\\r"; break; // Carriage return
            case '\t': result += "\\t"; break; // Tab
            default:
            if (std::iscntrl(static_cast<unsigned char>(c))) {
                // For other control characters (0x00-0x1F), use \u00XX
                std::stringstream ss;
                ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
                result += ss.str();
            } else {
                result += c;
            }
            break;
        }
    }
    return result;
}
void escape_all (user_dispatcher& UD)
{
    /* rib_24.12.2025 доп.экран. символа ' для pgsql */
    boost::replace_all(UD.jira_task.category, "'", "''");
    boost::replace_all(UD.jira_task.consumer_name, "'", "''");
    boost::replace_all(UD.jira_task.description, "'", "''");
    boost::replace_all(UD.jira_task.project, "'", "''");
    boost::replace_all(UD.jira_task.section, "'", "''");
    boost::replace_all(UD.jira_task.subj, "'", "''");

    /* Внутри labels (т.е. в секции) запрещены пробелы */
    boost::replace_all(UD.jira_task.section, " ", "");

    UD.jira_task.category = escape_json_entire_text(UD.jira_task.category);
    UD.jira_task.consumer_name = escape_json_entire_text(UD.jira_task.consumer_name);
    UD.jira_task.description = escape_json_entire_text(UD.jira_task.description);
    UD.jira_task.project = escape_json_entire_text(UD.jira_task.project);
    UD.jira_task.section = escape_json_entire_text(UD.jira_task.section);
    UD.jira_task.subj = escape_json_entire_text(UD.jira_task.subj);
}

static std::size_t extra_space(const std::string& s) noexcept
{
    std::size_t result = 0;

    for (const auto& c : s)
    {
        switch (c)
        {
        case '"':
        case '\\':
        case '\b':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
        {
            // from c (1 byte) to \x (2 bytes)
            result += 1;
            break;
        }

        default:
        {
            if (c >= 0x00 and c <= 0x1f)
            {
                // from c (1 byte) to \uxxxx (6 bytes)
                result += 5;
            }
            break;
        }
        }
    }
    return result;
}

/*!
@brief escape a string

Escape a string by replacing certain special characters by a sequence of an
escape character (backslash) and another character and other control
characters by a sequence of "\u" followed by a four-digit hex
representation.

@param[in] s  the string to escape
@return  the escaped string

@complexity Linear in the length of string @a s.
*/
std::string escape_string(const std::string& s) noexcept
{
    const auto space = extra_space(s);
    if (space == 0)
    {
        return s;
    }

    // create a result string of necessary size
    std::string result(s.size() + space, '\\');
    std::size_t pos = 0;

    for (const auto& c : s)
    {
        switch (c)
        {
        // quotation mark (0x22)
        case '"':
        {
            result[pos + 1] = '"';
            pos += 2;
            break;
        }

        // reverse solidus (0x5c)
        case '\\':
        {
            // nothing to change
            pos += 2;
            break;
        }

        // backspace (0x08)
        case '\b':
        {
            result[pos + 1] = 'b';
            pos += 2;
            break;
        }

        // formfeed (0x0c)
        case '\f':
        {
            result[pos + 1] = 'f';
            pos += 2;
            break;
        }

        // newline (0x0a)
        case '\n':
        {
            result[pos + 1] = 'n';
            pos += 2;
            break;
        }

        // carriage return (0x0d)
        case '\r':
        {
            result[pos + 1] = 'r';
            pos += 2;
            break;
        }

        // horizontal tab (0x09)
        case '\t':
        {
            result[pos + 1] = 't';
            pos += 2;
            break;
        }

        default:
        {
            if (c >= 0x00 and c <= 0x1f)
            {
                // print character c as \uxxxx
                sprintf(&result[pos + 1], "u%04x", int(c));
                pos += 6;
                // overwrite trailing null character
                result[pos] = '\\';
            }
            else
            {
                // all other characters are added as-is
                result[pos++] = c;
            }
            break;
        }
        }
    }

    return result;
}

std::string return_current_time_and_date()
{
    const auto now = std::chrono::system_clock::now();
    const auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%d-%m-%Y %X");
    return " "+ ss.str();
}

std::string itoa(const long l)
{
    std::stringstream ss;
    ss << l;
    std::string retString = ss.str();
    return retString;
};

/* curl's */
size_t callbackfunction(void *ptr, size_t size, size_t nmemb, void* userdata)
{
    FILE* stream = (FILE*)userdata;
    if (!stream)
    {
        c_out << "!!! No stream" << std::endl;
        return 0;
    }

    const size_t written = fwrite((FILE*)ptr, size, nmemb, stream);
    return written;
}

std::string download_image_from_url(const char* url, std::string fname)
{
    std::erase(fname, '/');
    std::erase(fname, '\\');

    std::string tmp_fname = "/tmp/" + QUuid::createUuid().toString().right(36).left(35).toStdString() + fname; /* убираем  '{'  и '}' из GUID */

    c_out <<  "Downloading to local " << tmp_fname << std::endl;
    FILE* fp = fopen(tmp_fname.c_str(), "w+b");
    if (!fp)
    {
        c_out << "Failed to create file on the disk " << tmp_fname << std::endl;
        return "";
    }

    CURL* curlCtx = curl_easy_init();
    curl_easy_setopt(curlCtx, CURLOPT_URL, url);
    curl_easy_setopt(curlCtx, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curlCtx, CURLOPT_WRITEFUNCTION, callbackfunction);
    curl_easy_setopt(curlCtx, CURLOPT_FOLLOWLOCATION, 1);

    const CURLcode rc = curl_easy_perform(curlCtx);
    if (rc)
    {
        std::cout << "Failed to download " << url << std::endl;
        return "";
    }

    long res_code = 0;
    curl_easy_getinfo(curlCtx, CURLINFO_RESPONSE_CODE, &res_code);
    if (!((res_code == 200 || res_code == 201) && rc != CURLE_ABORTED_BY_CALLBACK))
    {
        std::cout << "Response code: " << res_code << std::endl;
        return "";
    }

    curl_easy_cleanup(curlCtx);

    fclose(fp);

    return tmp_fname;
}

std::string strformat(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    auto length = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);

    if (length < 0) {
        abort();
    }

    auto buffer = (char*)malloc(length + 1);
    if (!buffer) {
        abort();
    }

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    const auto result = std::string(buffer, length);

    free(buffer);

    return result;
}

std::string sha1(const std::string& input)
{
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_length = 0;

    auto sha1 = EVP_MD_CTX_new();
    EVP_DigestInit_ex(sha1, EVP_sha1(), nullptr);
    EVP_DigestUpdate(sha1, input.c_str(), input.size());
    EVP_DigestFinal_ex(sha1, hash, &hash_length);
    EVP_MD_CTX_free(sha1);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_length; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

pgdb_issue* get_selected_issue_id(const int32_t msg_id, std::vector<pgdb_issue> &issues, const std::vector<int> &arr_recent_message_ids, const std::vector<most_recent_msgs> &recent_msgs)
{
    const auto iter = std::find(arr_recent_message_ids.begin(), arr_recent_message_ids.end(), msg_id);
    if (iter != arr_recent_message_ids.end())
    {
        int selectedButton = iter - arr_recent_message_ids.begin();
        int selectedIssueId = recent_msgs[selectedButton].ms_query_row_id;

        for (auto issueDB = issues.begin(); issueDB != issues.end(); ++issueDB)
        {
            if (issueDB->id == selectedIssueId)
            {
                /* returnsrc/s reference to the coresponding pointer of an iterator
                   msdbIssue* pointer_inside_buffer = &(*issueDB);                   */
                return &(*issueDB);
            }
        }
    }
    return nullptr;
}

bool delete_issue_from_db(const QSqlDatabase* db, const QString issue_id)
{
    /* И удаляем карточку из нашей БД */
    QSqlQuery Qtuery(*db);
    const auto res = Qtuery.exec("DELETE FROM issues WHERE idCard = '" + issue_id + "'");
    return res;
}

/* Handler, чтобы qDebug писал свой вывод в файл */
void bg_message_handler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    QString txt;
    switch (type)
    {
    case QtDebugMsg:
        txt = QString("%1").arg(msg);
        break;

    /*  case QtWarningMsg:
        txt = QString("Warning: %1").arg(msg);
        break;
    */
    case QtCriticalMsg:
        txt = QString("Critical: %1").arg(msg);
        break;
    case QtFatalMsg:
        txt = QString("Fatal: %1").arg(msg);
        abort();
    }

    const QDateTime date = QDateTime::currentDateTime();
    const QString formattedTime = date.toString("dd.MM.yyyy_hh");
    //QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();

    const QString logFileName{"chanserv-" + formattedTime + ".log"};

    const std::string chanserv_log_directory = qgetenv("CHANSERV_LOGDIR").toStdString();

    const std::string logFileLocation = chanserv_log_directory + "/" + logFileName.toStdString();

    QFile outFile(QString::fromStdString(logFileLocation));

    if (txt != "" && !msg.startsWith("QML Debugger: Waiting for connection on port") )
    {
        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(&outFile);
        /* ts << QTime::currentTime().toString() << " " << txt << Qt::endl; */
        ts << " " << txt << Qt::endl;
    }
}

void colour_out(const std::string _text)
{
    std::cout << return_current_time_and_date() << ": " << "\e[40m\e[37m " << _text << " \e[49m\e[39m" << std::endl;
    qDebug() << QString::fromStdString(return_current_time_and_date()) << ": " << QString::fromUtf8(_text);
}

void send_msg_to_support(const TgBot::Api &api, const std::string support_tg_id, const std::string additional_info, user_dispatcher &UD, const InlineKeyboardMarkup::Ptr &kbd)
{
    api.sendMessage(support_tg_id, additional_info + " " + UD.jira_task.section + " " +
                                                        UD.jira_task.project + " " +
                                                        UD.jira_task.subj    + " " +
                                                        UD.jira_task.creation_date  +
                                                        UD.jira_task.description   + " от " +
                                                        UD.jira_task.consumer_name,  nullptr, nullptr, kbd);
}

void send_msg_to_support(const TgBot::Api &api, const std::string support_tg_id, const std::string additional_info, const InlineKeyboardMarkup::Ptr &kbd)
{
    api.sendMessage(support_tg_id, additional_info,  nullptr, nullptr, kbd);
}

void send_msg_to_support(const TgBot::Api &api, const std::string support_tg_id, const std::string additional_info)
{
    api.sendMessage(support_tg_id, additional_info);
}

void restart_list_checker_in_qterminal(const std::string start_path)
{
    const QStringList args = QStringList() << "listchecker";

    QProcess::startDetached("pkill", args); /* Запускаем ListChecker в отдельном окне терминала */
    QProcess::startDetached("qterminal", QStringList() << "-e" << QString::fromStdString(start_path));
    c_out << "[Пере]запуск listchecker.sh" << std::endl;
}

void send_msg_to_staff(const TgBot::Api &api, std::vector<long> StaffTgIDs, const std::string MsgText, const InlineKeyboardMarkup::Ptr &kbd)
{
    c_out << "Отправляем уведомление о событии \"Заказчик-Исполнитель\" ответственным сотрудникам админ.аппарата: " << MsgText << std::endl;
    int i = 0;
    foreach (const long employee, StaffTgIDs) {
        /* Во время рассылок оборачиваем отправку в try/catch,
         * если бот в ЧС => TgException 403 ("Forbidden: bot was blocked by the user")  */
        try {
            c_out << itoa(i) + ") Отправка сообщения для " + itoa(employee) << std::endl;
            if (kbd)
                api.sendMessage(employee, MsgText, nullptr, nullptr, kbd);
            else
                api.sendMessage(employee, MsgText, nullptr, nullptr);
        } catch (TgException e) {
            c_out << e.what() + itoa(employee) << std::endl;
        }
        sleep(1); /* перерыв - спим секунду, чтобы бота не забанили как за спам-рассылку */
        ++i;
    }
    c_out << "Рассылка дублируемой информации завершена." << std::endl;
}
/*
trello_result<int> msg_to_deep_transformer(std::string full_url, const TgBot::Api &api, long sender_tg_id, user_dispatcher* t_uD, const std::string msg_text)
{
//  curl https://domain.name/{token}  -d '{"model": "gpt-4o-mini", "messages": [{"role": "user", "content": "Weather in Dallas"}], "temperature": 0, "max_tokens": 8}'

    const std::string MessageText = std::regex_replace(msg_text, std::regex("[\r\t\n]+" ), " ");

    std::string *jsonResp = new std::string;

    CURL *curl_req = curl_easy_init();
    if (!curl_req) {
        return trello_error::unknown;
    }

    curl_easy_setopt(curl_req, CURLOPT_HTTPPOST, 1);

    // Header
    curl_slist *postHeaderList = nullptr;
    postHeaderList = curl_slist_append(postHeaderList, HEADER_CONTENT_TYPE_APP_JSON);

    curl_easy_setopt(curl_req, CURLOPT_HTTPHEADER, postHeaderList);

    std::string strJSON = " {\"model\": \"gpt-4o-mini\", \"messages\": [{\"role\": \"user\", \"content\": \" ";
    strJSON.append(MessageText);
    strJSON.append(" \" }], \"temperature\": 0     } ");                                                                // \"max_tokens\": 28} ");

    curl_easy_setopt(curl_req, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl_req, CURLOPT_WRITEFUNCTION, curlWriteDataGPT);
    curl_easy_setopt(curl_req, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl_req, CURLOPT_POSTFIELDS, strJSON.c_str());

    const int res = curl_easy_perform(curl_req);

    long status_code = 0;
    if (res == CURLE_OK || res == 56)
    /* CURL status code 56 means "Passing data to be uploaded in URL itself instead of POST request"
    {
        /* c_out << "res = CURLE_OK, so => curl_easy_getinfo(curl_req, CURLINFO_RESPONSE_CODE, 0)" << std::endl;
        curl_easy_getinfo(curl_req, CURLINFO_RESPONSE_CODE, &status_code);
    }
    else {
        c_out << "curl_easy_perform's result wasn't OK, code: " + itoa(res) << std::endl;
    }

    if (status_code < 200 || status_code >= 300)
        c_out << &"HTTP Status Code: " [ status_code] << std::endl;

    if (status_code == 404)
    {
        c_out << "NotFound_404, exiting..." << std::endl;
        return trello_error::not_found_404;
    }

    if ( (status_code < 200 || status_code >= 300) && status_code != 56) {
        c_out << "Request failed " + itoa(res) + " status code " + itoa(status_code) << std::endl;
        return trello_error::request_failed;
    }

    boost::json::error_code ec;
    const boost::json::value responseJSON = boost::json::parse(jsonResp->c_str(), ec);
    if (ec.value() != 0) {
        c_out << "Boost failed to parse request's JSON" << std::endl;
        return trello_error::request_invalid;
    }

    std::string transformersAnswer = "";

    try
    {
        transformersAnswer = value_to<std::string>(responseJSON.at("choices").at(0).at("message").at("content"));
        c_out << value_to<std::string>(responseJSON.at("choices").at(0).at("message").at("content")) << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    try {
        api.sendMessage(sender_tg_id, transformersAnswer);
    } catch (TgException e) {
        api.sendMessage(sender_tg_id, "Блок try/catch: Сообщение " + MessageText + " трансформеру НЕ отправлено");
    }

/* 04.04.25  t_uD->jiraTask  = {};  /* Очистили структуру

    if(jsonResp)
        delete jsonResp;

    return 0;
}
*/

void send_document_to_all(const TgBot::Api &api, std::deque<long> ReceiverIDs, const std::string fileUrl, const std::string MsgText)
{
    c_out << "Рассылка файла..." << std::endl;
    foreach (const long receiver, ReceiverIDs) {
        try {
            c_out << "Отправка файла для " + itoa(receiver) << std::endl;
//          api.sendDocument(receiver, fileUrl, "", MsgText);
        } catch (TgException e) {
            c_out << e.what() + itoa(receiver) << std::endl;
        }

        sleep(1);
    }
}

void cout_small_talk(user_dispatcher &UD, const std::string fieldName, const Message::Ptr msg)
{
    std::cout << return_current_time_and_date() << ": " << UD.chat_user_tg_name << " (" << itoa(UD.chat_user_tg_id) << ") " << fieldName << ": " << msg->text.c_str() << std::endl;
    qDebug() << return_current_time_and_date() << UD.chat_user_tg_name << " (" << itoa(UD.chat_user_tg_id) << ") " << fieldName << ": " << msg->text.c_str();
}

/* utf8's U+FFFD (replacement character) is EVERYWHERE!!1! */
std::string truncate_msg_from_user(std::string str, const size_t width)
{
    QString qstr = QString::fromStdString(str);
    if (str.length() > width)
    {
        qstr = qstr.leftJustified(width, '.', true);
        qstr += "...";
    }
    return qstr.toStdString();
}

void send_email(const std::string& recipient, const std::string& script_path, const std::string& url)
{
    /* Первым параметром идёт, собственно, сам скрипт ps1 */
    QStringList args = QStringList() << QString::fromStdString(script_path);

    args.push_back(QString::fromStdString(recipient));
    args.push_back(QString::fromStdString("Веб-портал Jira"));
    args.push_back(QString::fromStdString(url));
    QProcess::startDetached("powershell", args);
    c_out << "E-mail " << url << " исполнителю " << recipient << " отправлен" << std::endl;
}

std::string generate_revit_master_key(const std::string &secret_key, const int digits, const int time_step)
{
    const QByteArray qba_SecretKey = QString::fromStdString(secret_key).toUtf8();

    /* rib_16_11_2025 заменено генератором псевдослучайного количества секунд Get current Unix time (seconds) qint64 currentUnixTime = QDateTime::currentSecsSinceEpoch(); */
    const qint64 currentUnixTime = QDateTime::currentSecsSinceEpoch();QRandomGenerator64::global()->generate();
    qint64 timeStepValue = currentUnixTime / time_step;

    /* Convert timeStepValue to big-endian 8-byte array */
    QByteArray timeStepBytes(8, '\0');
    for (int i = 7; i >= 0; --i) {
        timeStepBytes[i] = static_cast<char>(timeStepValue & 0xFF);
        timeStepValue >>= 8;
    }

    /* Compute HMAC-SHA1 using Qt
     * (Qt’s QCryptographicHash doesn’t do HMAC directly, so we’ll implement the standard HMAC manually) */
    const int blockSize = 64; // Block size for SHA1
    QByteArray key = qba_SecretKey;

    if (key.size() > blockSize)
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha1);
    if (key.size() < blockSize)
        key.append(QByteArray(blockSize - key.size(), '\0'));

    QByteArray o_key_pad(blockSize, '\x5c');
    QByteArray i_key_pad(blockSize, '\x36');

    for (int i = 0; i < blockSize; ++i) {
        o_key_pad[i] = o_key_pad[i] ^ key[i];
        i_key_pad[i] = i_key_pad[i] ^ key[i];
    }

    const QByteArray innerHash = QCryptographicHash::hash(i_key_pad + timeStepBytes, QCryptographicHash::Sha1);
    const QByteArray hmac = QCryptographicHash::hash(o_key_pad + innerHash, QCryptographicHash::Sha1);

    // Dynamic truncation
    const int offset = hmac[hmac.size() - 1] & 0x0F;
    const int binary = ((hmac[offset] & 0x7F) << 24) |
                 ((hmac[offset + 1] & 0xFF) << 16) |
                 ((hmac[offset + 2] & 0xFF) << 8) |
                 (hmac[offset + 3] & 0xFF);

    const int otp = binary % static_cast<int>(std::pow(10, digits));

    // Format with leading zeros
    const auto rret =  QString("%1").arg(otp, digits, 10, QChar('0'));
    return rret.toStdString();
}
