#pragma once

#include "abstract_chanserv.h"
#include "trelloapi.h"
#include "structures.h"
#include "support.h"
#include "jiraapi.h"
#include "simpleini.h"
/*
#include "maxbotapi.h"
#include "tm_bot.hpp"
*/
#include <iostream>

#include <QtSql>
#include <QSqlDatabase>
#include <QTcpServer>
#include "qhttpserver.h"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QtDBus>
#include <QHostInfo>
#include <tgbot/tgbot.h>
#include <tgbot/Bot.h>
#include <tgbot/types/InlineKeyboardMarkup.h>
#include <tgbot/types/Message.h>
#include <tgbot/types/ReplyKeyboardMarkup.h>
#include <tgbot/net/HttpClient.h>

using namespace TgBot;

class chanserv_bot : public AbstractChanserv
{
    int create_new_issue(Message::Ptr message, user_dispatcher* t_uD);
    int receive_photos(Message::Ptr message, user_dispatcher* t_uD);
/*  void tm_run_bot_api(manapi::maxbot &tm_bot, const std::string& t_bot_token);    */
public:
    explicit chanserv_bot();
    /*  ChanservBot(const ChanservBot&) noexcept = default;
        ChanservBot(const ChanservBot&&) noexcept = default;      */
    chanserv_bot& operator=(chanserv_bot const&) = delete;
    chanserv_bot& operator=(const chanserv_bot&&) = delete;
    bool operator()(std::string t) = delete;
    ~chanserv_bot();

    Bot m_tg_bot;    
/*
    manapi::maxbot tm_bot;
    std::shared_ptr<manapi::async::context> m_context;
    MessageQueue<std::string> msg_queue;
    std::unordered_map<int64_t, MessageQueue<std::string>> chat_message_queues;
    ChatSessions *chat_sessions;
*/
    jira_api* m_jira_api;
    trello_api* m_trello_api;

    /* Блок уникальных данных для каждого юзера */
    user_dispatcher* uD;
    std::vector<user_dispatcher*> v_uD;
    /* ======================================== */

    ReplyKeyboardMarkup::Ptr  kb_ganzel;         /* Основная non-inline клавиатура в строке ввода */
/*  ReplyKeyboardMarkup::Ptr  keyboardGuest;
    ReplyKeyboardMarkup::Ptr  keyboardConsumer;   */
    InlineKeyboardMarkup::Ptr  kb_developer;

    InlineKeyboardMarkup::Ptr kb_manage_photos;
    InlineKeyboardMarkup::Ptr kb_consumer;
    InlineKeyboardMarkup::Ptr kb_contractor;
    InlineKeyboardMarkup::Ptr kb_staff;
    InlineKeyboardMarkup::Ptr kb_tekla;
    InlineKeyboardMarkup::Ptr kb_start_solving;
    InlineKeyboardMarkup::Ptr kb_start_solving_w_photo;
    InlineKeyboardMarkup::Ptr kb_task_closing;
    InlineKeyboardMarkup::Ptr kb_task_done;
    InlineKeyboardMarkup::Ptr kb_go_dashboard;

    InlineKeyboardButton::Ptr btn_go_dashboard;
    InlineKeyboardButton::Ptr btn_start_solving;
    InlineKeyboardButton::Ptr btn_more_photos;
    InlineKeyboardButton::Ptr btn_task_closing;
    InlineKeyboardButton::Ptr btn_task_done;
    InlineKeyboardButton::Ptr btn_no_more_photos;

    InlineKeyboardButton::Ptr btn_new_task;
    InlineKeyboardButton::Ptr btn_new_family;
    InlineKeyboardButton::Ptr btn_my_tasks;
    InlineKeyboardButton::Ptr btn_new_tekla;
    InlineKeyboardButton::Ptr btn_edit_msi_version;
    InlineKeyboardButton::Ptr btn_add_new_contractor;    

    InlineKeyboardButton::Ptr btn_improve_suggest;
    InlineKeyboardButton::Ptr btn_model_repair;    
    InlineKeyboardButton::Ptr btn_directory_access;
    InlineKeyboardButton::Ptr btn_general_issue;
    InlineKeyboardButton::Ptr btn_tekla_back;

    InlineKeyboardButton::Ptr btn_send_file;
    InlineKeyboardButton::Ptr btn_run_shell_command;
/*  InlineKeyboardButton::Ptr btnRebootHost;        */
    InlineKeyboardButton::Ptr btn_msg_as_bot;
    InlineKeyboardButton::Ptr btn_forward_to_gpt;

/*  InlineKeyboardButton::Ptr btnReqMasterKey; */
    std::vector<InlineKeyboardButton::Ptr> consumer_buttons;
    std::vector<InlineKeyboardButton::Ptr> contractor_buttons;
    std::vector<InlineKeyboardButton::Ptr> staff_buttons;
    std::vector<InlineKeyboardButton::Ptr> dev_buttons;

    void bot_start(QCoreApplication *app);
    int  IInitializeDB() override;
    void initialize_tracking_apis();
    void initialize_commands();

    void session_dispatcher();   /* Диспетчер чат-сеансов для разделения пользователей и их "бесед" */

    void on_start_command(Message::Ptr message);
    void handle_commands();
    void on_more_photos_command(pgdb_issue const &issue, CallbackQuery::Ptr query);

    void on_callback_query();
    void on_new_task_query(CallbackQuery::Ptr query);
    void on_new_family_query(CallbackQuery::Ptr query);

    void handle_small_talk();
    void get_my_issues(const long from_message_id);

    int create_keyboards();
    void create_one_column_keyboard(const std::vector<std::string>& button_strings, ReplyKeyboardMarkup::Ptr& kb);
    void create_keyboard(const std::vector<std::vector<std::string>>& button_layout, ReplyKeyboardMarkup::Ptr& kb);

    bool is_developer(const long id);
    bool is_db_consumer(const long id);
    bool is_staff(const long id);
    bool is_allowed_contractor(const long id);
    const std::string insert_revit_key_to_db(QSqlDatabase* odb, const user_dispatcher* t_uD);
    int  add_consumer_to_db(QSqlDatabase* odb, long tg_id, std::string tg_link);
    int  write_issue_to_db(QSqlDatabase* hwnd_db, const user_dispatcher* ud, int table_uniq);

    /* Фоновой процесс со служебными задачами */
    void qt_bg_worker(QElapsedTimer* timer);
    void jira_webhooks_handler(const std::string& request_url);

    void service_notification(const std::string& msg_notify);
    void service_notification_coroutine(const std::string& msg_notify, boost::coroutines2::coroutine<void>::push_type& yield);

    /* Разделяем пользовательские сессии не потоками и не, прости Господи - корутинами */
    user_dispatcher* uniq_data(const long tg_id);
};
