#include "chanserv.h"
#include <boost/json/src.hpp> /* must be used in a single source file, it should not be in a header file, you should use #include <boost/json.hpp> instead */

#ifdef Q_OS_LINUX
    #include "adaptorimp.h"
    #include "serverdbus.h"
#endif

#include <regex>


using namespace TgBot;

chanserv_bot::chanserv_bot(): m_tg_bot(sett["tg_bot_token"]) /* the reverse proxy set in Bot.h constructor (parameter name is const std::string& "url") */
{
    const std::vector<std::string> mssql_settings = {"msidbhost", "msidb", "msidbusr", "msidbpasswd"};
    foreach (const auto key, mssql_settings) {
        msdb[key] = get_setting(key.c_str());
    }

#ifdef Q_OS_LINUX

/* ========= DBUS Server, связь c приложением ListChecker =============================================== */

    /* Create the object that implement the adaptor */
    //AdaptorImp* list_checker_adaptor = new AdaptorImp;
    std::unique_ptr<AdaptorImp> list_checker_adaptor(new AdaptorImp);

    // Pass the object to the adaptor constructor
    new ServerDBus(&*list_checker_adaptor);

    // Register the object on the session bus
    QDBusConnection connection = QDBusConnection::sessionBus();
    const bool ret = connection.registerService("chanserv.qdbus");
    const auto rret = connection.registerObject("/chanserv", &*list_checker_adaptor);
    c_out << "Chanserv: DBus адаптер для связи с приложением ListChecker зарегистрирован" << std::endl;

#endif
}

/*
void chanserv_bot::tm_run_bot_api(manapi::maxbot &tm_bot, const std::string& t_bot_token)
{
    ChatSessions chatSessions(tm_bot, m_tg_bot.getApi(), m_context);
    tm_bot.token(t_bot_token);

    // manapi::init_tools::log_trace_init(manapi::debug::LOG_TRACE_LOW);

    manapi::async::context::threadpoolfs(2);
    // manapi::async::context::gbs = manapi::async::context::blockedsignals();

    m_context = manapi::async::context::create(2).unwrap();

    m_context->eventloop()->setup_handle_interrupt();

    m_context->run(m_context, 0, [this, tm_bot, &chatSessions] (auto callbackFn) -> void {
            std::set<size_t> states;

            manapi::async::run ([&states, this, tm_bot, &chatSessions] () mutable MANAPIHTTP_NOEXCEPT
                                -> manapi::future<> {

            auto json_buttons = manapi::json {
                  { "text", "У вас появилась новая задача" },
                  { "attachments", manapi::json::array({
                    {
                         { "type", "inline_keyboard" },
                         { "payload", {
                              { "buttons", manapi::json::array({
                                manapi::json::array({
                                {
                                   { "type", "link" },
                                   { "text", "Открыть Jira" },
                                   { "url", sett["jira_project_url"] }
                                }
                                , //2nd button
                                {
                                   { "type", "callback" },
                                   { "text", "Press me!" },
                                   { "payload", "button1 pressed" }
                                }

                                })
                                }
                              )}
                            }
                          }
                    }
                  })}
                };

                auto res_send = co_await tm_bot.send_message(910345284460, json_buttons);

                tm_bot.on("message_created", [&states, &chatSessions](manapi::maxbot &self, manapi::json &data)
                        -> manapi::future<bool> {

                    if (!chatSessions.hasSession(data["message"]["sender"]["user_id"].as_integer())) {
                        std::cout << "Creating new chat session for chat " << data["message"]["sender"]["user_id"].as_integer() << std::endl;
                        chatSessions.createNewSession(data["message"]["sender"]["user_id"].as_integer());
                    }

                    auto const user_id = data["message"]["sender"]["user_id"].as_integer();
                    auto const chat_id = data["message"]["recipient"]["chat_id"].as_integer();
                    auto const &text = data["message"]["body"]["text"];

                    chatSessions.passMessage(data["message"]["sender"]["user_id"].as_integer(), text.as_string());

                    if (text.is_string() && text.as_string().starts_with("@k_o_s_m_o_s_bot"))
                    {
                        // is a personal addressing
                        std::string_view const public_text{text.as_string().data() + 17, text.as_string().size() - 17};
                        std::cout << public_text << std::endl;
                        //if (public_text == "/start")
                        {
                            auto res_send = co_await self.send_message(user_id, chat_id, {
                                                {"text", std::format("Заполнение занятости\nНачиная с 10:00 вы можете выполнить заполненение занятости!", "") }  });
                            manapi::json response = (res_send.unwrap());
                        }
                    }
                    if (!text.is_string()
                        || !text.as_string().starts_with('/'))
                        co_return false;

                    // is a command
                    std::string_view const cmd{text.as_string().data() + 1, text.as_string().size() - 1};

                    if (cmd == "start") {
                        auto res_send = co_await self.send_message(user_id, chat_id, { {"text", std::format("Hello, {}. You are here.\nWhat do you want to ask me?", "" )}  });
                        manapi::json response = (res_send.unwrap());
                    }
                    else if (cmd == "ai enable") {
                        states.insert(user_id);
                    }
                    else if (cmd == "ai disable") {
                        states.erase(user_id);
                    }
                    else {
                        auto send_res = co_await self.send_message(user_id, chat_id, { {"text", std::format("Invalid command: {}", cmd)}});
                        manapi::json response = (send_res).unwrap();
                    }

                    co_return true;
                });
                tm_bot.bind(500).unwrap();

            });

            callbackFn();
        }).unwrap();

    manapi::clear_tools::curl_library_clear();
    manapi::clear_tools::ev_library_clear();
    manapi::clear_tools::ssl_library_clear();
}
*/

void chanserv_bot::initialize_commands()
{
    /* LEFT MENU COMMANDS ================================================================== */

    std::vector<BotCommand::Ptr> v_commands;
    BotCommand::Ptr cmd_array;
    cmd_array = std::shared_ptr<BotCommand>(new BotCommand);
    cmd_array->command = "start";
    cmd_array->description = "Перезапустить бота";
    v_commands.push_back(cmd_array);

    cmd_array = std::shared_ptr<BotCommand>(new BotCommand);
    cmd_array->command = "help";
    cmd_array->description = "Справка";
    v_commands.push_back(cmd_array);

    c_out << "Initializing '/' commands..." << std::endl;
//    m_tg_bot.getApi().setMyCommands(v_commands);

    /* LEFT MENU COMMANDS ================================================================== */
}

int chanserv_bot::IInitializeDB()
{
    c_out << "Chanserv: Connecting to MSSQL DB " + msdb["msidbhost"] + "\\" + msdb["msidb"] + " as user " + msdb["msidbusr"] << std::endl;
    QString conn_str = "DRIVER={ODBC Driver 18 for SQL Server};Encrypt=Optional;TrustServerCertificate=Yes;"; /* the driver name taken from /etc/odbcinst.ini   */
    conn_str.append("Server=" + QString::fromStdString(msdb["msidbhost"]) + ";");
    conn_str.append("Database=" + QString::fromStdString(msdb["msidb"]) + ";");        /* DB name   */
    conn_str.append("Uid=" + msdb["msidbusr"] + ";");                                  /* Username  */
    conn_str.append("Pwd=" + QString::fromStdString(msdb["msidbpasswd"]) + ";");       /* Password  */
    h_msDB.setDatabaseName(conn_str);

    const bool rret = h_msDB.open();
    if (!rret)
    {
        c_out << h_msDB.lastError().text().toStdString() << std::endl;
        throw;
    }

    return 0;
}

void chanserv_bot::initialize_tracking_apis()
{
    c_out << "Starting APIs: Trello, Jira..." << std::endl;
    m_trello_api = new trello_api(sett["trello_key"], sett["trello_token"], sett["trello_prefix"]);
    m_jira_api = new jira_api(sett["jira_token"], sett["jira_base_url"], m_tg_bot.getApi(), sett["support_TgID"], pgDatabase, sett["script_path_mail_sender"]);
}

void chanserv_bot::bot_start(QCoreApplication *app)
{

/*                  chatSessions = new ChatSessions(tm_bot, m_tg_bot.getApi(), m_Context); */
// TODO: MAXBOT     chatSessions = new ChatSessions(m_tg_bot.getApi());

    session_dispatcher();    /* Диспетчер чат-сеансов для разделения пользователей и их "бесед" */

    /* Handler for "help", "start" commands */
    handle_commands();

    on_callback_query();  /* v2.0: Введем инлайновые кнопки вместо команд /XXXX_XXX */

    /* Self the message exchange */
    handle_small_talk();

#ifdef Q_OS_LINUX
    signal(SIGINT, [](int s)
    {
        std::cout << "ChanservBot got SIGINT!" << s << std::endl;
        qDebug() << "LastError: " << errno << "\n";
        exit(1);
    });
#endif

    c_out << "Bot username: " << m_tg_bot.getApi().getMe()->username.c_str() << std::endl;

    c_out << "Skipping all pending updates" << std::endl;
    m_tg_bot.getApi().deleteWebhook(true); /* true for dropPendingUpdates/skip msg updates */

/*  in_thread tm_run_bot([this]{ tm_run_bot_api(tm_bot, sett["t_bot_token"]); });
    c_out << "MaxBot NOT started!" << std::endl;    */

    while (true)
    {
        TgLongPoll long_poll(m_tg_bot);
        c_out << "Long poll started" << std::endl;
        while (true)
        {
            // app->processEvents();   /* we need this as QDBus won't work without it */
            long_poll.start();
        }
    }
    /* webhook run
    {
        try
        {
            TgWebhookTcpServer webhook_server(8080, m_tgBot);

            colour_out("Please note: the tg token is also set in nginx_proxy_webhook.conf!");
            m_tgBot.getApi().setWebhook("https://e.xxxxxxx.dev:8443");

            auto webhook_info = m_tgBot.getApi().getWebhookInfo();
            c_out << "ssh -R 8080:localhost:8080 user@e.xxxxxxx.dev -p XX" << std::endl;
            webhook_server.start();
        }
        catch (std::exception &e){
            c_out << "BotStart's webhook error: " + std::string(e.what()) << std::endl;
        }
    }
    */
//   co_return;
}

int chanserv_bot::receive_photos(Message::Ptr message, user_dispatcher* t_uD)
{
    c_out << "Status is SHARED.FIO.STEP5 and message's text is empty. Прислали фото?" << std::endl;
    try
    {
        File::Ptr file;
        if (!message->photo.empty())
        {
           unsigned char biggest_photo = message->photo.size() - 1;
            {
                c_out << t_uD->chat_user_tg_name + " (" + itoa(t_uD->chat_user_tg_id) + ") Получено изображение с ID " + message->photo[biggest_photo]->fileId << std::endl;
                file = m_tg_bot.getApi().getFile(message->photo[biggest_photo]->fileId);
                if (file->filePath != "")
                {
                    c_out << "Tg's picture: " << file->filePath << std::endl;
                    std::string photo_url = "https://api.telegram.org/file/bot";
                    photo_url += sett["tg_bot_token"] + "/" + file->filePath;

                    const char *image_url = photo_url.data();
                    t_uD->jira_task.v_local_file_paths.push_back(download_image_from_url(image_url, file->filePath));
                }
                t_uD->jira_task.photoIDs += message->photo[biggest_photo]->fileId + "\\n";
            }
        }

        if (message->document)
        {
            c_out << t_uD->chat_user_tg_name + " (" + itoa(t_uD->chat_user_tg_id) + ") Received a file, file ID: " + message->document->fileId << std::endl;
            file = m_tg_bot.getApi().getFile(message->document->fileId);
            t_uD->jira_task.v_docu_ids.push_back(message->document->fileId + "\n");
            if (file->filePath != "")
            {
                std::cout << file->filePath << std::endl;
                std::string photo_url = "https://api.telegram.org/file/bot";
                photo_url += sett["tg_bot_token"];
                photo_url.append("/");
                photo_url += file->filePath;

                const char *image_url = photo_url.data();

                t_uD->jira_task.v_local_file_paths.push_back(download_image_from_url(image_url, file->filePath));
            }
        }
    }
    catch (std::exception &e) {
        c_out << "Фото находится в чате. Ошибка: " << e.what() << std::endl;
    }

    return 0;
}

int chanserv_bot::create_new_issue(Message::Ptr message, user_dispatcher* t_uD)
{
    m_tg_bot.getApi().sendMessage(message->chat->id, "Записано. Отправляем задачу в БД...");
    const std::chrono::time_point time_point = std::chrono::system_clock::now() + std::chrono::hours(3);
    const auto str_dattime  = std::format(std::locale::classic(), "{:%d.%m.%Y %H:%M}", time_point);
    t_uD->jira_task.creation_date = str_dattime;

    std::string card_description = "https://t.me/" + message->chat->username + " ";
    card_description += t_uD->jira_task.consumer_name + "\n";

    /* В случае, если задача это Создание семейства, то может быть заполнена Категория. Пишем её тоже в карточку */
    if (t_uD->jira_task.category != ""){
        card_description += "Категория: " + t_uD->jira_task.category + "\n";
    }

    t_uD->created_card_id = nullptr;
    /* Результат лежит в t_uD->createdCardID:    */

    /* Экранируем всё, что попадёт в query */
    escape_all(*t_uD);

    if (t_uD->jira_task.type != "TEKLA"){
        try
        {   /* Добавляем карточку с новым заданием в Trello */
            if (!m_trello_api->create_new_card(sett["distribution_list"],
                                          t_uD->jira_task.section + " " + t_uD->jira_task.project + " " + t_uD->jira_task.subj + " " + t_uD->jira_task.creation_date,
                                          card_description + t_uD->jira_task.description, &t_uD->created_card_id).is_ok())
            {
                send_msg_to_support(m_tg_bot.getApi(), sett["support_TgID"], "Ошибка создания карточки " + card_description, *t_uD, kb_go_dashboard);
                c_out << "Ошибка создания карточки. Сообщение разработчику отправлено." << std::endl;
            }
        }
        catch (std::exception e)
        {
            std::cout << "Ошибка создания карточки: " << e.what() << std::endl;
            send_msg_to_support(m_tg_bot.getApi(), sett["support_TgID"], "Ошибка создания карточки " + QString::fromLocal8Bit(e.what()).toStdString() + "\n" + card_description, *t_uD, kb_go_dashboard);
        }

        t_uD->jira_task.id_card = *t_uD->created_card_id;

        try
        {
            /* Saving the task to snapshot's database as well */
            int table = table_bim_issues;
            if (t_uD->jira_task.type == "TEKLA")
                table = table_tekla_issues;

            if (write_issue_to_db(&pgDatabase, t_uD, table) != 0)
            {
                send_msg_to_support(m_tg_bot.getApi(), sett["support_TgID"], "Ошибка записи в базу данных.", *t_uD, kb_go_dashboard);
                c_out << "Ошибка записи в базу данных. Сообщение supportTgID отправлено." << std::endl;
            }
        }
        catch (std::exception e)
        {
            send_msg_to_support(m_tg_bot.getApi(), sett["support_TgID"], "Ошибка записи в базу данных PostgreSQL.", *t_uD, kb_go_dashboard);
            c_out << "Ошибка записи в базу данных PostgreSQL. Сообщение разработчику отправлено." << std::endl;
        }

        /* Аттачим все присланные изображения в карточку Trello */
        if (t_uD->created_card_id != nullptr)
        {
            if (!t_uD->jira_task.v_local_file_paths.empty())
                for (int i = 0; i < t_uD->jira_task.v_local_file_paths.size(); ++i)
                    auto ret = m_trello_api->add_img_to_card(*t_uD->created_card_id, t_uD->jira_task.v_local_file_paths[i]);

            c_out << "Карточка " << *t_uD->created_card_id << " создана." << std::endl;

            /* Освобождаем память, std::string createdCardID был allocated в ф-ции trelloAPI->createNewCard() */
            delete t_uD->created_card_id;
        }

        /* Уведомление админ.аппарата о появлениии новой задачи */
        const std::string subj_copy = t_uD->jira_task.subj;
        const std::string consumer_name_copy = t_uD->jira_task.consumer_name;
        in_thread notify_staff([this, subj_copy, consumer_name_copy]{ send_msg_to_staff(m_tg_bot.getApi(), v_staff, "На доске появилась новая задача " + subj_copy + " от " + consumer_name_copy, kb_go_dashboard); });
    }

    m_tg_bot.getApi().sendMessage(message->chat->id, "Ваша задача поступила в работу, скоро с вами свяжутся. Для нового обращения нажмите на клавиатуре /start"
                                                  ""); /* , nullptr, nullptr, kbInlineConsumer); */
    m_tg_bot.getApi().sendSticker(message->chat->id, *select_randomly(sticker_list.begin(), sticker_list.end()));  /* randomize! */



    /* pgInsertToDB(t_uD); */

    std::string proj_name = "USERTASKS";    
    if (t_uD->jira_task.type == "TEKLA")
        proj_name = "TEKLA";
    /* Jira's PART ================================================================================================*/
    try
    {   /* Добавляем карточку с новой задачей в Jira */
        const int res_issue_num = m_jira_api->create_task(t_uD, proj_name);
        if (res_issue_num < 3)
        {
            send_msg_to_support(m_tg_bot.getApi(), sett["support_TgID"], "Ошибка создания задачи Jira " + card_description, *t_uD, kb_go_dashboard);
            c_out << "Ошибка создания задачи Jira. Сообщение supportTgID отправлено." << std::endl;
            t_uD->last_error = 1;
        }
        else
        {
            /* Аттачим все присланные изображения в задачу Jira */
            if (!t_uD->jira_task.v_local_file_paths.empty())
                for (int i = 0; i < t_uD->jira_task.v_local_file_paths.size(); ++i)
                    auto ret = m_jira_api->add_attach(t_uD->jira_task.v_local_file_paths[i], proj_name + "-" + itoa(res_issue_num));
            if (t_uD->jira_task.type == "TEKLA")
                m_tg_bot.getApi().sendMessage(sett["tekla_admin_tgid"], "B Jira появилась новая задача: vm-srv051.local:8443/browse/TEKLA-" + itoa(res_issue_num));
        }
    }
    catch (std::exception e)
    {
        c_out << "Ошибка создания задачи Jira: " << e.what() << std::endl;
        send_msg_to_support(m_tg_bot.getApi(), sett["support_TgID"], "Ошибка создания задачи Jira " + QString::fromLocal8Bit(e.what()).toStdString() + card_description, *t_uD, kb_go_dashboard);
        t_uD->last_error = 1;
    }
/*
        manapi::async::context::current(m_Context);
        manapi::async::run ( [this] () mutable -> manapi::future<> {
            auto rret = co_await tm_bot.send_message(atol(sett["m_developer_id"].c_str()), { { "text", std::format("B Jira появилось новое обращение") } });
         co_return; });
*/
    /* Jira's PART ================================================================================================*/


    /* Очистили структуру */
    t_uD->jira_task = {};
    /*      if(t_uD->lastError != 0)
                {
                    t_uD->lastError = 0;
                    t_uD->jiraTask  = {};  /* Очистили структуру
                }
                else    */
    return 0;
}

/* Coroutine setup: */
void chanserv_bot::service_notification(const std::string& msg_notify)
{
    // Create the coroutine
    boost::coroutines2::coroutine<void>::pull_type coroutine(
        [this, msg_notify](boost::coroutines2::coroutine<void>::push_type& yield) {
            service_notification_coroutine(msg_notify, yield);
        });

    // Execute the coroutine steps
    while(coroutine) {
        /* std::cout << "Processing next notification..." << std::endl; */
        /* Resumes the coroutine until next yield */
        coroutine();
    }
    /*  std::cout << "All notifications processed!" << std::endl;   */
}

void chanserv_bot::service_notification_coroutine(const std::string& msg_notify, boost::coroutines2::coroutine<void>::push_type& yield)
{
    QSqlQuery qt_query(pgDatabase);
    qt_query.exec("SELECT * FROM consumers");
    int i = 0;
    while(qt_query.next())
    {
        const int id = qt_query.value(0).toInt();
        QString username = qt_query.value(1).toString();
        // tgBot.getApi().sendMessage(usrnam, msgNotify);
        /* testing to see if the console will return any of the entry_ids from the database */

        // Yield control back to caller (simulates async behavior)
        yield();

        std::this_thread::sleep_for(std::chrono::seconds(1));
        ++i;
        c_out << itoa(i) + ") emulated Sent '" + msg_notify + "' to " + username.toStdString() + " (" + itoa(id) + ")" << std::endl;
    }
    qt_query.clear();

    c_out << "Рассылка сообщения завершена" << std::endl;
}

/* Возвращаем из ф-ции все данные написавшего в чат юзера: new_task, issues, recent messages etc */
user_dispatcher* chanserv_bot::uniq_data(long tg_id)
{
    user_dispatcher* ret = nullptr;
    foreach (auto itm, v_uD) {
        if (itm->chat_user_tg_id == tg_id)
            ret = itm;
    }
    return ret;
}

const std::string chanserv_bot::insert_revit_key_to_db(QSqlDatabase* odb, const user_dispatcher* t_uD)
{
    QSqlQuery ms_query(*odb);

    const QString strQuery = "SELECT TOP 1 master_key FROM master_keys WHERE tg_id = '" + QString::number(t_uD->chat_user_tg_id) + "' AND is_activated = 0";
    ms_query.clear();
    ms_query.exec(strQuery);

    int requests_count = ms_query.numRowsAffected();
    c_out << "Количество запросов на одобрение от юзера " + itoa(t_uD->chat_user_tg_id) + ": " + itoa(requests_count) << std::endl;
    /* В таблице не существует открытых запросов на разблокировку/одобрение */
    if (requests_count == 0)
    {
        ms_query.clear();
        std::chrono::time_point time_point = std::chrono::system_clock::now() + std::chrono::hours(3);
        const auto str_date_time = std::format(std::locale::classic(), "{:%d.%m.%Y %H:%M}", time_point);

        const std::string generated_revit_key = generate_revit_master_key(sett["fam_otp_key"]);
        const QString to_masterkey_tab = "INSERT INTO master_keys ( tg_id, master_key, requested_by, requested_at, reason, is_activated ) VALUES('" +
                                          QString::number(t_uD->chat_user_tg_id) + "', " +
                                    "'" + QString::fromStdString(generated_revit_key) + "', " +
                                    "'" + QString::fromStdString(t_uD->jira_task.consumer_name) + "', " +
                                    "'" + QString::fromStdString(str_date_time) + "', " +
                                    "'" + QString::fromStdString(t_uD->jira_task.key_reason) + "', 0);";

        try
        {
            if(!ms_query.exec(to_masterkey_tab))
                std::cout << odb->lastError().text().toStdString() << std::endl;
            if(!odb->commit())
                std::cout << odb->lastError().text().toStdString() << std::endl;
        }
        catch (std::exception e)
        {
            std::cout << e.what() << std::endl;
            return "";
        }

        return generated_revit_key;
    }
    /* Если есть открытый запрос, то возвращаем мастер ключ */
    else
    {
        ms_query.next();
        return ms_query.value(0).toString().toStdString();
    }
    return "";
}

int chanserv_bot::add_consumer_to_db(QSqlDatabase* odb, long tg_id, const std::string tg_link)
{
    if (std::find(v_consumers.begin(), v_consumers.end(), tg_id) != v_consumers.end()){}
    else
    {
        QSqlQuery pg_query(*odb);
        const QString ins_consumers_table = "INSERT INTO consumers ( id, username ) VALUES('" + QString::number(tg_id) + "', " +
            "'" + QString::fromStdString(tg_link) + "');";

        try
        {
            if(!pg_query.exec(ins_consumers_table))
                std::cout << odb->lastError().text().toStdString() << std::endl;
            if(!odb->commit())
                std::cout << odb->lastError().text().toStdString() << std::endl;
        }
        catch (std::exception e)
        {
            std::cout << e.what() << std::endl;
            return 1;
        }
    }
    return 0;
}

int chanserv_bot::write_issue_to_db(QSqlDatabase* hwnd_db, const user_dispatcher* ud, const int table_uniq)
{
    std::string issues_table_name = "issues";
    /* АТЕНСЬОН! The maximum length of a Telegram message is 4096 characters (utf8) */
    QSqlQuery pg_query(*hwnd_db);
    if (table_uniq == table_tekla_issues)
        issues_table_name = "tekla_issues";

    const std::string ins_issues_table = "INSERT INTO " + issues_table_name +
    " ( consumer_id,    \
        consumer_name,  \
        consumer_link,  \
        section,        \
        category,       \
        theme,          \
        description,    \
        created_time,   \
        idcard,         \
        card_status,    \
        photos,         \
        project         \
                       )\
    VALUES('"
       + itoa(ud->jira_task.consumer_tg_id)             + "', " +
        "'" + ud->jira_task.consumer_name               + "', " +
        "'" + ud->jira_task.consumer_tg_username_link   + "', " +
        "'" + ud->jira_task.section                     + "', " +
        "'" + ud->jira_task.category                    + "', " +
        "'" + ud->jira_task.subj                        + "', " +
        "'" + ud->jira_task.description                 + "', " +
        "'" + ud->jira_task.creation_date               + "', " +
        "'" + ud->jira_task.id_card                     + "', " +
        "'pending', " +
        "'" + ud->jira_task.photoIDs                    + "', " +
        "'" + ud->jira_task.project                     + "');";

    try
    {
        if(!pg_query.exec(QString::fromStdString(ins_issues_table))){
            std::cout << "sqlexec: " << hwnd_db->lastError().text().toStdString() << std::endl;
            std::cout << "psqlexec: " << pg_query.lastError().text().toStdString() << std::endl;
            return 1;
        }
        /* Для PostgreSQL autocommit=1 сообщит "WARNING: there is no transaction in progress" */
        if(!hwnd_db->commit()){
            std::cout << "sqlcommit: " << hwnd_db->lastError().text().toStdString() << std::endl;
            return 1;
        }
    }
    catch (std::exception e)
    {
        std::cout << "sqlexception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void chanserv_bot::jira_webhooks_handler(const std::string& request_url)
{
    m_tg_bot.getApi().sendMessage(sett["support_TgID"], "JIRA: " + request_url);
    const size_t pos = request_url.find_last_of("/");
    // make sure the position is valid
    const std::string issue_key = request_url.substr(pos + 1);
    if (request_url.substr(1, 3) == "UPD")
        m_jira_api->get_task_info(issue_key);
}

void chanserv_bot::qt_bg_worker(QElapsedTimer* timer)
{
    const auto bot_mem_size = get_mem_size(sett["script_path_getmem_chanserv"]);

    const std::string uptime = itoa(timer->elapsed()/1000/60);

    m_tg_bot.getApi().sendMessage(sett["support_TgID"], "TgBot " + itoa(bot_mem_size) + " Мб в памяти" + ", uptime " + uptime + " min");

/* Если процесс listchecker'а закиляли, то запускаем его   */
    if (get_proc_id_by_name("listchecker") <= 0)
        restart_list_checker_in_qterminal(sett["script_path_listchecker"]);
}

chanserv_bot::~chanserv_bot()
{
    h_msDB.close();

    chanserv_ini_.Reset();

    /* delete trelloAPI; */
}

bool chanserv_bot::is_developer(const long id)
{
    if (id != atol(sett["support_TgID"].c_str()))
        return false;
    else return true;
}

bool chanserv_bot::is_db_consumer(const long id)
{
    if (std::find(v_consumers.begin(), v_consumers.end(), id) != v_consumers.end())
        return true;
    else return false;
}

bool chanserv_bot::is_staff(const long id)
{
    if (std::find(v_staff.begin(), v_staff.end(), id) != v_staff.end())
        return true;
    else return false;
}

bool chanserv_bot::is_allowed_contractor(const long id)
{
    if (std::find(v_allowed_contractors.begin(), v_allowed_contractors.end(), id) != v_allowed_contractors.end())
        return true;
    else return false;
}

void chanserv_bot::get_my_issues(const long fromMessageId)
{
    /* АТЕНСЬОН! Shorten the title length: Reduce the image title to less than 200 characters. */
    uD = uniq_data(fromMessageId);
    if (uD)
    {
        QSqlQuery pg_query(pgDatabase);

        const QString str_query = "SELECT \
            id, consumer_link, consumer_name, section, category, theme, description, created_time, idCard, \
            photos, project, contractor_list, contractor_link, contractor_name  \
                                   FROM issues \
                                   WHERE contractor_id = '" + QString::number(fromMessageId) + "'";

        pg_query.clear();
        pg_query.exec(str_query);
        int pg_tasks_count = pg_query.numRowsAffected();
        c_out << "pg, tasks count: " << itoa(pg_tasks_count) << std::endl;

        std::cout << return_current_time_and_date() << ": " << uD->chat_user_first_name << " @" << uD->chat_user_tg_name << " (" << fromMessageId << ") запросил свои задачи. У него их ";

        pgdb_issue pg_db_issue;

        /* при каждом нажатии на Мои задачи мы очищаем кэш прошлого нажатия */
        uD->recent_msgs.clear();
        uD->recent_msg_ids.clear();
        uD->pgdb_issues.clear();

        int p = 0;
        while(pg_query.next())
        {
            pg_tasks_count = pg_query.numRowsAffected();

            if (pg_tasks_count > 0)
            {
                {
                    pg_db_issue.id            = pg_query.value(0).toInt();
                    pg_db_issue.from          = pg_query.value(2).toString().toStdString() + " (@" + pg_query.value(1).toString().toStdString() + ")"; /* Name of user and his tg-link */
                    pg_db_issue.unit          = pg_query.value(3).toString().toStdString(); /* Section */
                    pg_db_issue.category      = pg_query.value(4).toString().toStdString();
                    pg_db_issue.theme         = pg_query.value(5).toString().toStdString(); /* Subj */
                    pg_db_issue.description   = pg_query.value(6).toString().toStdString();
                    pg_db_issue.creation_date  = pg_query.value(7).toString().toStdString();  /* Created time */
                    pg_db_issue.id_card = pg_query.value(8).toString().toStdString();        /* Trello's card */
                    QStringList pg_photos_list = pg_query.value(9).toString().split("\\n", Qt::SkipEmptyParts);

                    for (int x = 0; x < pg_photos_list.count(); ++x)
                    {
                        pg_db_issue.photos.push_back(pg_photos_list[x].toStdString());
                    };

                    pg_db_issue.project        = pg_query.value(10).toString().toStdString();
                    pg_db_issue.contractor_list = pg_query.value(11).toString().toStdString();
                    pg_db_issue.contractor_link = pg_query.value(12).toString().toStdString();   /* Исполнитель задачи */
                    pg_db_issue.contractor_name = pg_query.value(13).toString().toStdString();
                }
                ++p;
                pg_db_issue.issue_num = "---------- Задача " + itoa(p) + " ----------";

                std::string built_msg =
                    "UIN: " + itoa(pg_db_issue.id)                    \
                    + "\nРаздел: " + pg_db_issue.unit                 \
                    + "\nТема: " +  pg_db_issue.theme                 \
                    + "\nПроект: " + pg_db_issue.project              \
                    + "\nОт кого: " + pg_db_issue.from                \
                    + "\nДата создания: " + pg_db_issue.creation_date  \
                    + "\nОписание: " + pg_db_issue.description;

                if (pg_db_issue.theme == "Создание семейства")
                {
                    built_msg =
                        "ID%: " + itoa(pg_db_issue.id) + " | " +  pg_db_issue.theme \
                        + "\nВыполнение строго в Revit 2022❗"             /* красный восклицательный знак */        \
                        + "\nРаздел: " + pg_db_issue.unit                 \
                        + "\nКатегория: " + pg_db_issue.category          \
                        + "\nПуть до тех.док.: " + pg_db_issue.project    \
                        + "\nОт кого: " + pg_db_issue.from                \
                        + "\nДата создания: " + pg_db_issue.creation_date  \
                        + "\nОписание: " + pg_db_issue.description;
                }

                Message::Ptr sent_msg;

                if (pg_db_issue.photos.empty())
                    sent_msg = m_tg_bot.getApi().sendMessage(fromMessageId, built_msg, nullptr, nullptr, kb_start_solving);

                /* Если сообщение с фото, то длина текста урезается с 4096 до 1024 */
                if (pg_db_issue.photos.size() >= 1 && built_msg.length() > 1000)
                {
                    built_msg = truncate_msg_from_user(built_msg, 1000);
                }
                if (pg_db_issue.photos.size() == 1)
                    sent_msg = m_tg_bot.getApi().sendPhoto(fromMessageId, pg_db_issue.photos[0], built_msg, nullptr, kb_start_solving);
                if (pg_db_issue.photos.size() > 1)
                    sent_msg = m_tg_bot.getApi().sendPhoto(fromMessageId, pg_db_issue.photos[0], built_msg, nullptr, kb_start_solving_w_photo);

                pg_db_issue.sent_msg_id = sent_msg->messageId;
                pg_db_issue.sent_chat_id = sent_msg->chat->id;
                uD->recent_msgs.push_back( {pg_db_issue.id, sent_msg->messageId} );
                uD->recent_msg_ids.push_back(sent_msg->messageId);
                uD->pgdb_issues.push_back(pg_db_issue);

                pg_db_issue = {} /* clear struct */;
            }
        }
        pg_query.clear();


        if (pg_tasks_count == 0)
        {
            m_tg_bot.getApi().sendMessage(fromMessageId, "У вас нет активных задач\nКайфуйте 🚬🚬🚬",  /* tobacco emojis */
                                   nullptr, nullptr, kb_ganzel);
            m_tg_bot.getApi().sendSticker(fromMessageId, "CAACAgIAAxkBAAEJ-3tk1di32sT158dYv7TgeRTQIU9c1wACsBYAAg5Z6Esr859Kked7aDAE");  /* Cillian Murphy smokes gif */
        }
    }
}

