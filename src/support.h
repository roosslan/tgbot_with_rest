#pragma once

#include "trelloapi.h"
#include "result.h"
#include "structures.h"

#include <random>
#include <iterator>
#include <functional>
#include <mutex>
#include <deque>
#include <tgbot/tgbot.h>
#include <curl/curl.h>
#include <unicode/utypes.h>

#include <QSqlDatabase>

#include <boost/json.hpp>
#include "boost/regex.hpp"
#include <boost/algorithm/string.hpp>

#include <boost/coroutine2/all.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>



using namespace TgBot;

typedef unsigned char BYTE;
typedef size_t curl_write_cb_t(void *contents, size_t size, size_t count, void *stream);

template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}
/* Шаблон для случайной выборки стикера */
template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen);
}

std::string return_current_time_and_date();

/* совместим std::cout с qDebug() */
class cpp_out
{
    std::string str_debug_to_log_ = "";
    std::ostream& my_own_cout_func_ = std::cout;

    bool prefix_printed = false;
    void add_prefix_if_needed() {
        if (!prefix_printed) {
            my_own_cout_func_ << return_current_time_and_date() + ": ";
            prefix_printed = true;
        }
    }
public:
    cpp_out& operator << (const std::string& sstr) {
        add_prefix_if_needed();
        my_own_cout_func_ << sstr;
        str_debug_to_log_ += sstr;
        return *this;
    }
    /* указатель на функцию, которая принимает ссылку на std::ostream и возвращает ссылку на std::ostream */
    cpp_out& operator << (std::ostream&(*ofs)(std::ostream&)) {
        add_prefix_if_needed();
        /* вызов функции, на которую указывает ofs, с аргументом m_cout */

        if (std::endl<char, std::char_traits<char>> == ofs) {
            qDebug() << QString::fromStdString(return_current_time_and_date()) << ": " << QString::fromUtf8(str_debug_to_log_);
            str_debug_to_log_ = "";
            prefix_printed = false;
        }

        ofs(my_own_cout_func_);
        return *this;
    }
};
/* std::cout to stdout and into log-file (using qDebug) wrapper */
static cpp_out c_out;

/* Повторяющийся код, вынесен в отд. ф-цию. Дублирует cout в консоль и в лог */
void cout_small_talk(user_dispatcher &UD, std::string fieldName, Message::Ptr msg);


/* A common idiom for clearing standard containers is swapping with an empty version of the container: */
template <typename StlContainer>
void clear_stl_container(StlContainer &container)
{
    StlContainer empty_cont;

    StlContainer temp = std::move(empty_cont);
    empty_cont = std::move(container);
    container = std::move(temp);

    /* или std::swap(container, emptyCont);  */
}

/* Ф-ция проверки - запущен ли процесс [listchecker'а] */
int get_proc_id_by_name(const std::string proc_name);

void restart_list_checker_in_qterminal(const std::string start_path);

void create_keyboard(const std::vector<std::vector<std::string>>& button_layout, ReplyKeyboardMarkup::Ptr& kb);

void create_one_column_keyboard(const std::vector<std::string>& button_strings, ReplyKeyboardMarkup::Ptr& kb);

/* Сообщаем разработчикам, если какие-то операции не удались */
void send_msg_to_support(const TgBot::Api &api, const std::string support_tg_id, const std::string additional_info, user_dispatcher& UD, const InlineKeyboardMarkup::Ptr &kbd);
void send_msg_to_support(const TgBot::Api &api, const std::string support_tg_id, const std::string additional_info, const InlineKeyboardMarkup::Ptr &kbd);
void send_msg_to_support(const TgBot::Api &api, const std::string support_tg_id, const std::string additional_info);

/* Различные уведомления о выполнении задачи для админов и пр. */
void send_msg_to_staff(const TgBot::Api &api, std::vector<long> StaffTgIDs, const std::string MsgText, const InlineKeyboardMarkup::Ptr &kbd = nullptr);

/* Рассылка файла с инструкцией */
void send_document_to_all(const TgBot::Api &api, std::deque<long> ReceiverIDs, const std::string fileUrl, const std::string MsgText);

/* Вывод (c_out) сообщения с выделением цветом */
void colour_out(const std::string _text);

/* Handler, чтобы qDebug писал свой вывод в файл */
void bg_message_handler(QtMsgType type, const QMessageLogContext &, const QString & msg);

bool delete_issue_from_db(const QSqlDatabase* db, QString issue_id);

/* Которую из полученных задач в списке нажал Исполнитель? */
pgdb_issue* get_selected_issue_id(const int32_t msg_id, std::vector<pgdb_issue> &issues, const std::vector<int> &arr_recent_message_ids, const std::vector<most_recent_msgs> &recent_msgs);

/* GPT предобученный трансформер
trello_result<int> msg_to_deep_transformer(std::string full_url, const TgBot::Api &api, long sender_tg_id, user_dispatcher* t_uD, const std::string msg_text);
*/

std::string remove_replacement_character(const std::string& input);
/* Обрезать строку до длины N */
std::string truncate_msg_from_user(std::string str, const size_t width);

std::string download_image_from_url(const char* url, std::string fname);

std::string itoa(const long l);

/* Ф-ции экранирования */
void escape_all (user_dispatcher& UD);
std::string escape_string(const std::string& s) noexcept;
std::string strformat(const char* fmt, ...);
std::string sha1(const std::string& input);

void send_email(const std::string &recipient, const std::string &script_path, const std::string &url);

/* Ф-ция для генерации OTP */
std::string generate_revit_master_key(const std::string &secret_key, const int digits = 6, const int time_step = 2);

template <typename Result>
static Result with_mutex(std::mutex& mutex, std::function<Result()> func)
{
    auto guard = std::lock_guard<std::mutex>(mutex);
    return func();
}

struct in_thread
{
    template <typename FN> in_thread(FN fn) : thread([this, fn] { fn(); }) {}
    std::thread thread;
    ~in_thread() {  thread.join(); }
    /*
     * inThread(const inThread&) = delete;
     * inThread(inThread&&) = delete;
     * inThread& operator= (inThread) = delete;
     * 
    */
};

/* Infinite loop in separate thread */
struct active_object
{
    template <typename Fn> active_object(Fn fn) : thread([this, fn] {   while(alive){ fn(); }   }) {}

    ~active_object() { alive = false; thread.join(); }

    active_object(const active_object&) = delete;
    active_object(active_object&&) = delete;
    active_object& operator= (active_object) = delete;

    std::atomic<bool> alive{ true };
    std::thread thread;
    void terminate(){ std::terminate(); };
};

static void with_mutex(std::mutex& mutex, const std::function<void()>& func)
{
    auto guard = std::lock_guard<std::mutex>(mutex);
    func();
}

#define LOG_TAG(name) static const char* TAG = #name
#define LOGE(tag, format, ...) printf("\033[31mERROR [%s] " format "\033[0m\n", tag, ##__VA_ARGS__)
#define LOGW(tag, format, ...) printf("\033[33mWARN [%s] " format "\033[0m\n", tag, ##__VA_ARGS__)
#define LOGI(tag, format, ...) printf("\033[97mINFO [%s] " format "\033[0m\n", tag, ##__VA_ARGS__)
#define LOGD(tag, format, ...) printf("DEBUG [%s] " format "\n", tag, ##__VA_ARGS__)

class curl_session
{
    CURL* session_;

public:
    curl_session(CURL* session) : session_(session) {}
    curl_session(const curl_session& other) = delete;
    curl_session(curl_session&& other) noexcept = delete;
    curl_session& operator=(const curl_session& other) = delete;
    curl_session& operator=(curl_session&& other) noexcept = delete;

    ~curl_session() {
        if (session_) {
            curl_easy_cleanup(session_);
        }
    }

    CURL* operator*() const { return session_; }
};

template <typename T>
class curl_data
{
    T _data;

public:
    curl_data(T session) : _data(session) {}
    curl_data(const curl_data& other) = delete;
    curl_data(curl_data&& other) noexcept = delete;
    curl_data& operator=(const curl_data& other) = delete;
    curl_data& operator=(curl_data&& other) noexcept = delete;

    ~curl_data() {
        if (_data) {
            curl_free(_data);
        }
    }

    T operator*() const { return _data; }
};
