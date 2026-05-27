#include "listchecker.hpp"

list_checker::list_checker(): tg_bot(sett["tg_bot_token"])
{
    /* ReadSettingsFromDB(); */

    connect(QDBusConnection::sessionBus().interface(),
            SIGNAL(service_owner_changed(QString,QString,QString)),
            this,SLOT(service_owner_changed(QString,QString,QString)));

/*    if (Bot_DBus)  Bot_DBus->RunCommand(      */
}

list_checker::~list_checker()
{
    /* Close system log */
    syslog(LOG_NOTICE, "Stopping ListChecker daemon");
    closelog();
}

void list_checker::bootstrap()
{
    kb_go_dashboard_ = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_go_dashboard_options;
    btn_go_dashboard_ = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_go_dashboard_->text = "К доске! 🚀";                       /* Rocket emoji */
    btn_go_dashboard_->url = sett["board_link"];
    row_go_dashboard_options.push_back(btn_go_dashboard_);
    kb_go_dashboard_->inlineKeyboard.push_back(row_go_dashboard_options);
}

/* Добавляем исполнителя в таблице БД "issues" */
void list_checker::assign_contractor_to_card(const std::string& sql_query, const std::string& card_id, contractor* p_contractor)
{
    QSqlQuery pg_query(pgDatabase);

    c_out << "Assign a contractor to the card: " + sql_query << std::endl;
    pg_query.exec(QString::fromStdString(sql_query));

    pgDatabase.commit();
    /* Пишем пользователю, кто будет делать его задачу */
    pg_query.clear();
    pg_query.exec("SELECT consumer_id FROM issues WHERE idCard = '" + QString::fromStdString(card_id) + "'");
    pg_query.next();
    const QString db_consumer_id = pg_query.value(0).toString();

    InlineKeyboardMarkup::Ptr kb_contact_contractor;
    InlineKeyboardButton::Ptr btn_contact_contractor;

    kb_contact_contractor = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_contact_contractor;
    btn_contact_contractor = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_contact_contractor->text = "Связаться ⚡️";
    btn_contact_contractor->url = "https://t.me/" + p_contractor->link;
    row_contact_contractor.push_back(btn_contact_contractor);
    kb_contact_contractor->inlineKeyboard.push_back(row_contact_contractor);

    /* Пишем пользователю */
    tg_bot.getApi().sendMessage(db_consumer_id.toULong(), "Вашу задачу выполнит " + p_contractor->name + "!\n", nullptr, nullptr, kb_contact_contractor);

    if (bot_dbus)
        bot_dbus->send_tg_message("Заказчику " + db_consumer_id + " отправлено сообщение: " + "Вашу задачу выполнит " + QString::fromStdString(p_contractor->name));

    /* Пишем исполнителю - emoji: три восклицательных знака */
    tg_bot.getApi().sendMessage(p_contractor->id, "❗️❗️❗️ \nУ вас появилась новая задача - проверьте свои задачи");

    if (bot_dbus)
        bot_dbus->send_tg_message("Исполнителю " + QString::number(p_contractor->id) + " отправлено сообщение: У вас появилась новая задача - проверьте свои задачи");

    c_out << "Новая задача от " + db_consumer_id.toStdString() + " . Уведомление отправлено исполнителю " + p_contractor->name << std::endl;
}

void list_checker::rework_card_and_notify(const std::string& card_id, const int card_index)
{
    struct row_consumer
    {
        QString db_subj;
        QString id;
        QString name;
        QString link;
    };

    row_consumer customer;

    QSqlQuery qt_query(pgDatabase);

    qt_query.exec("SELECT theme, consumer_id, consumer_name, consumer_link FROM issues WHERE idCard = '" + QString::fromStdString(card_id) + "'");
    qt_query.next();
    customer.db_subj = qt_query.value(0).toString();
    customer.id = qt_query.value(1).toString();
    customer.name = qt_query.value(2).toString();
    customer.link = qt_query.value(3).toString();

    tg_bot.getApi().sendMessage(customer.id.toULong(), "Извините, но наши специалисты вас не поняли.\nСформулируйсте вопрос подробнее и создайте новую задачу");
    in_thread notify_staff([this, customer]{
        send_msg_to_staff(tg_bot.getApi(), v_staff,
                       "Уведомление 'Cоздайте новую задачу' юзеру " + customer.name.toStdString() + " по задаче " + customer.db_subj.toStdString() + " отправлено", kb_go_dashboard_); });


    if (bot_dbus)
        bot_dbus->send_tg_message("Заказчику " +  customer.name + " (" + customer.link + ") отправлено сообщение: Извините, но наши специалисты вас не поняли. Сформулируйсте вопрос подробнее и создайте новую задачу");

    /* Архивируем карточку на доске Trello */
    p_trello->archiving_card(card_id);
    c_out << "Карточка " + card_id + " находилась на доске 'Переделать' и теперь архивирована" << std::endl;

    /* Удаляем карточку из БД */
    delete_issue_from_db(&pgDatabase, QString::fromStdString(card_id));

    colour_out("Карточка " + v_id_cards_[card_index] + " удалена из БД");

    pgDatabase.commit();
}

/* Синхр с БД: если карточку вернули обратно в первую колонку, и ее надо очистить от исполнителя */
std::string list_checker::sync_distributed_card(const std::string card_id)
{
    QSqlQuery qt_query(pgDatabase);
    qt_query.exec("SELECT idCard_list FROM issues WHERE idCard = '" + QString::fromStdString(card_id) + "'");
    qt_query.next();
    const QString db_card_list_id = qt_query.value(0).toString();
    /* if(dbCardListId == ""){
                c_out << "null: SELECT idCard_list FROM issues WHERE idCard = '" + cardId.toStdString() + "'" << std::endl; } */
    qt_query.clear();
    qt_query.exec("SELECT contractor_list FROM issues WHERE idCard = '" + QString::fromStdString(card_id) + "'");
    qt_query.next();
    const QString db_contractor_list = qt_query.value(0).toString();
    if(db_contractor_list == "")
    {
        c_out << "Исполнитель не назначен: SELECT contractor_list FROM issues WHERE idCard = '" + card_id + "'" << std::endl;
    }

    if ( (db_card_list_id == QString::fromStdString(sett["distribution_list"])) && (db_contractor_list != "") )
    {
        c_out << "Карточка " + card_id + " находится на доске для распределения и при этом поле Исполнитель не пустое => задача была возвращена обратно -" << std::endl;
        const QString queryRemoveContractorFromCard = "UPDATE issues SET card_status = 'pending', contractor_list = '', \
            contractor_link = '', contractor_id = '', contractor_name = '' \
                                          WHERE idCard = '" + QString::fromStdString(card_id) + "'";
                                            qt_query.clear();
        c_out << "Built and carrying out an SQL-query to remove a contractor from the card: " + queryRemoveContractorFromCard.toStdString() << std::endl;
        auto ret = qt_query.exec(queryRemoveContractorFromCard);
    }

    return db_card_list_id.toStdString();
}

/* Функции проверки карточек на доске и БД. В случае различий - выполнение соотв.действий (sync, оичстка или удаление) */
void list_checker::sync_n_manage_contractors_and_cards(const std::string card_id, std::string trello_card_list_id, int card_index)
{
    QSqlQuery qt_query(pgDatabase);
    /* Проверяем - если карточку вернули обратно в первую колонку, то её надо очистить от исполнителя */
    const std::string db_card_list_id = sync_distributed_card(card_id);

    /* Проверка соответствия колонки исполнителя и фактической колонки - если колонки совпали, то пишем исполнителю о том что у него появилась задача */
    qt_query.clear();
    qt_query.exec("SELECT card_status FROM issues WHERE idCard = '" + QString::fromStdString(card_id) + "'");
    qt_query.next();
    const QString db_card_status = qt_query.value(0).toString();

    contractor contractor_of_card;
    std::string query_assign_contractor_to_card = "";
    if (db_card_status == "pending")
    {
        /* Итерация по вектору исполнителей */
        for (auto it = v_contractors.begin(); it != v_contractors.end(); ++it)
        {
            if (it->trello_list == trello_card_list_id)
            {
                query_assign_contractor_to_card = "UPDATE issues SET card_status = 'underway', contractor_list = '" + it->trello_list + "', " +
                " contractor_link = '" + it->link + "', contractor_id = '" + itoa(it->id) + "', contractor_name = '" + it->name + "'" +
                    " WHERE idCard = '" + card_id + "'";
                contractor_of_card = *it;
                break;
            };
        }

        if (query_assign_contractor_to_card != "")  /* Means the Card assigned to some Contractor */
        {
            /* Добавляем исполнителя в таблице БД "issues" */
            assign_contractor_to_card(query_assign_contractor_to_card, card_id, &contractor_of_card);
            query_assign_contractor_to_card = "";
        }
        else
        {
            c_out << "Пропускаем карточку " + card_id + "... Запрашиваем следующую..." << std::endl;
        }
        /* Если карточка находится в колонке "Переделывай!", пишем пользователю */
        if (QString::fromStdString(db_card_list_id) == QString::fromStdString(sett["rework_list"]))
        {
            rework_card_and_notify(card_id, card_index);
        }
    }
}

void list_checker::checks_bootstrap(QElapsedTimer* timer)
{
    c_out << "Getting all cards from pgDB into an array..." << std::endl;

    QSqlQuery pg_query(pgDatabase);
    clear_stl_container(v_id_cards_);     /* Очищаем контейнер, т.к. он созд. не здесь */

    pg_query.clear();
    pg_query.exec("SELECT idCard FROM issues");
    std::string q_db_last_error = pgDatabase.lastError().text().toStdString();
    std::string q_last_error = pg_query.lastError().text().toStdString();
    while(pg_query.next()){
        std::string dbcard = pg_query.value(0).toString().toStdString();
        if (dbcard != "1")
            v_id_cards_.push_back(dbcard);
    }

    if (v_id_cards_.empty())
    {
        c_out << "Запрос задач из pgDB вернул пустой результат. " +
                     pgDatabase.lastError().text().toStdString() + " " + pg_query.lastError().text().toStdString() << q_db_last_error + " " + q_last_error <<  std::endl;

        /* std::chrono::milliseconds(timer->elapsed()); */
        int upt_minutes = timer->elapsed()/1000/60;
        if (upt_minutes > 29){
            if (upt_minutes % 30 == 0)    /* Не спамим разработчику об ошибке каждые 30 секунд. Но сообщаем об ошибке каждые 30 минут */
            {
                tg_bot.getApi().sendMessage(sett["support_TgID"], "ListChecker's uptime is " + itoa(upt_minutes) + "min: Запрос задач из БД вернул пустой результат. " +
                                                                     pgDatabase.lastError().text().toStdString() + " " + pg_query.lastError().text().toStdString() + q_db_last_error + " " + q_last_error);
//              ReconnectDB();
            }
        }

        const QVariantMap varMap;
        bot_dbus->run_command(QString::fromStdString(sett["script_path_listchecker"]), varMap);
    }
}

void list_checker::process_every_card(QSqlQuery* pg_query)
{
    for (int i = 0; i  < v_id_cards_.size(); ++i )
    {
        const auto trello_card = p_trello->get_card(v_id_cards_[i]);
        if (trello_card.is_ok() && !trello_card.is_error(trello_error::not_found_404)
            && !trello_card.is_error(trello_error::request_failed)
            && !trello_card.is_error(trello_error::request_invalid)
            && !trello_card.is_error(trello_error::unknown)            )
        {
            std::cout << return_current_time_and_date() << ": Собираем карточки, закрытые на доске, чтобы их удалить. К удалению: ";
            if(trello_card.value().is_closed) /* 0 or 1 */
                v_cards_to_delete_.push_back(v_id_cards_[i]);
            std::cout << itoa(v_cards_to_delete_.size()) << " карточек" << std::endl;

            /* Блок с запросом в какой колонке карточка, записываем результат в БД */
            const QString trello_card_list_id = QString::fromStdString(trello_card.value().id_list);
            const QString card_id             = QString::fromStdString(trello_card.value().id);
            const QString card_name           = QString::fromStdString(trello_card.value().name);

            /* First and foremost, SELECT followed by UPDATE is almost always incorrect under concurrency.
             * Unless things run under SERIALIZABLE isolation level, there is no guarantee that rows don't change between the SELECT and the UPDATE.
             * Second, for the cases when there are rows to update, the cost of SELECT + UPDATE is by definition higher than just an UPDATE.        */
            const QString query_update_cards_id_list = "UPDATE issues SET idCard_list = '" + trello_card_list_id + "' WHERE idCard = '" + card_id + "'";
            pg_query->clear();
            c_out << query_update_cards_id_list.toStdString() << std::endl;
            pg_query->exec(query_update_cards_id_list);

            /* Функции проверки карточек на доске и БД. В случае различий - выполнение соотв.действий (sync, оичстка или удаление) */
            sync_n_manage_contractors_and_cards(card_id.toStdString(), trello_card_list_id.toStdString(), i);
        }

        /* Если карточку удалили на доске, то удаляем её из БД */
        if (trello_card.is_error(trello_error::not_found_404)) /* NB: коммит не вызывается */
            if(delete_issue_from_db(&pgDatabase, QString::fromStdString(v_id_cards_[i])))
                colour_out("Карточка " + v_id_cards_[i] + " не найдена на доске и поэтому удалена из БД");

        pgDatabase.commit();

    }
}

void list_checker::check_the_list(QElapsedTimer* timer)
{
    /* LEGACY: QSqlDatabase requires a QCoreApplication, which requires an event loop must run in the thread the QCoreApplication object was created in. */

    /* DBUS Client, связь c ботом =============================================== */

    bot_dbus = std::shared_ptr<client_dbus>(new client_dbus("chanserv.qdbus", "/chanserv", QDBusConnection::sessionBus(), 0));

 /*  DBus section is commented out due to migration onto Docker */
 /*
 * if (!bot_dbus->connection().interface()->isServiceRegistered("chanserv.qdbus"))
{
    c_out << "Не подключена POSIX-шина межпроцессного взаимодействия DBus для связи с ботом. chanserv (бот) запущен?" << std::endl;
    c_out << "Системный DBus-монитор d-feet может показать подробности о DBus-службе 'chanserv.qdbus'." << std::endl;
    if(bot_dbus){
        c_out << "Служба \"DBus\" на \"" << QHostInfo::localHostName().toStdString() << "\" была запущена и затем остановлена. Некоторые службы автоматически останавливаются, если они не используются другими службами или программами." << std::endl;
    }
    c_out << "Спим " + itoa(LISTCHECK_DURATION_IN_MSEC) + " миллисекунд..." << std::endl;

    int upt_minutes = timer->elapsed()/1000/60;
    if (upt_minutes > 29){
        if (upt_minutes % 30 == 0)    // Не спамим разработчику об ошибке каждые 30 секунд. Но сообщаем об ошибке каждые 30 минут
            tg_bot.getApi().sendMessage(sett["support_TgID"], "ListChecker's uptime is " + itoa(upt_minutes) + "min: Не подключена POSIX-шина межпроцессного взаимодействия DBus для связи с ботом. chanserv (бот) запущен? ");
    }
}
else */
{
    checks_bootstrap(timer);
//    c_out << "Getting all cards from the database into an array..." << std::endl;

    QSqlQuery qt_query(pgDatabase);
    clear_stl_container(v_id_cards_);     /* Очищаем контейнер, т.к. он созд. не здесь */

    qt_query.clear();
    qt_query.exec("SELECT idCard FROM issues");
    std::string q_db_last_error = pgDatabase.lastError().text().toStdString();
    std::string q_last_error = qt_query.lastError().text().toStdString();
    while(qt_query.next()){
        std::string dbcard = qt_query.value(0).toString().toStdString();
        if (dbcard != "1")
            v_id_cards_.push_back(dbcard);
    }

   if (v_id_cards_.empty())
    {
        c_out << "Запрос задач из БД вернул пустой результат. " +
                    pgDatabase.lastError().text().toStdString() + " " + qt_query.lastError().text().toStdString() << q_db_last_error + " " + q_last_error <<  std::endl;

       /* std::chrono::milliseconds(timer->elapsed()); */
       int upt_minutes = timer->elapsed()/1000/60;
        if (upt_minutes > 29){
           if (upt_minutes % 30 == 0)    /* Не спамим разработчику об ошибке каждые 30 секунд. Но сообщаем об ошибке каждые 30 минут */
            {
                tg_bot.getApi().sendMessage(sett["support_TgID"], "ListChecker's uptime is " + itoa(upt_minutes) + "min: Запрос задач из БД вернул пустой результат. " +
                pgDatabase.lastError().text().toStdString() + " " + qt_query.lastError().text().toStdString() + q_db_last_error + " " + q_last_error);
//                ReconnectDB("re");
           }
        }

       const QVariantMap vari_map;
        bot_dbus->run_command(QString::fromStdString(sett["script_path_listchecker"]), vari_map);
    }
   else {
    process_every_card(&qt_query);
    for (int i = 0; i  < v_id_cards_.size(); ++i )
    {
        const auto trello_card = p_trello->get_card(v_id_cards_[i]);
        if (trello_card.is_ok() && !trello_card.is_error(trello_error::not_found_404)
            && !trello_card.is_error(trello_error::request_failed)
            && !trello_card.is_error(trello_error::request_invalid)
            && !trello_card.is_error(trello_error::unknown)            )
        {
            std::cout << return_current_time_and_date() << ": Собираем карточки, закрытые на доске, чтобы их удалить. К удалению: ";
            if(trello_card.value().is_closed) /* 0 or 1 */
                v_card_ids_to_delete_.push_back(v_id_cards_[i]);
            std::cout << itoa(v_card_ids_to_delete_.size()) << " карточек" << std::endl;

            /* Блок с запросом в какой колонке карточка, записываем результат в БД */
            const QString trelloCardListId = QString::fromStdString(trello_card.value().id_list);
            const QString cardId           = QString::fromStdString(trello_card.value().id);
            const QString cardName         = QString::fromStdString(trello_card.value().name);

            /* First and foremost, SELECT followed by UPDATE is almost always incorrect under concurrency.
             * Unless things run under SERIALIZABLE isolation level, there is no guarantee that rows don't change between the SELECT and the UPDATE.
             * Second, for the cases when there are rows to update, the cost of SELECT + UPDATE is by definition higher than just an UPDATE.        */
            const QString query_update_cards_id_list = "UPDATE issues SET idCard_list = '" + trelloCardListId + "' WHERE idCard = '" + cardId + "'";
            qt_query.clear();
            c_out << query_update_cards_id_list.toStdString() << std::endl;
            qt_query.exec(query_update_cards_id_list);

            /* Функции проверки карточек на доске и БД. В случае различий - выполнение соотв.действий (sync, оичстка или удаление) */
            sync_n_manage_contractors_and_cards(cardId.toStdString(), trelloCardListId.toStdString(), i);
        }

        /* Если карточку удалили на доске, то удаляем её из БД */
        if (trello_card.is_error(trello_error::not_found_404)) /* NB: коммит не вызывается */
            if(delete_issue_from_db(&pgDatabase, QString::fromStdString(v_id_cards_[i])))
                colour_out("Карточка " + v_id_cards_[i] + " не найдена на доске и поэтому удалена из БД");

        pgDatabase.commit();

    }/* for... every Card */ }

    delete_cards(&qt_query);

    if (!v_card_ids_to_delete_.empty())
    {
        std::string str_card_ids_to_delete = "";

        for (auto & element : v_card_ids_to_delete_)
            str_card_ids_to_delete.append("'" + element + "', ");
        str_card_ids_to_delete.pop_back();
        str_card_ids_to_delete.pop_back();

        qt_query.clear();
        const QString qstr_card_ids_to_delete = QString::fromStdString ("DELETE FROM issues WHERE idCard IN (" + str_card_ids_to_delete + ")" );
        c_out << qstr_card_ids_to_delete.toStdString() << std::endl;
        colour_out("Карточки, закрытые на доске, были удалены");
        qt_query.exec(qstr_card_ids_to_delete);

        /* Очищаем список карточек к удалению быстро swap'ом */
        clear_stl_container(v_card_ids_to_delete_);

    }
    pgDatabase.commit();
    c_out << "Спим " + itoa(LISTCHECK_DURATION_IN_MSEC) + " миллисекунд..." << std::endl;
  }
}

void list_checker::delete_cards(QSqlQuery* pg_query)
{
    if (!v_cards_to_delete_.empty())
    {
        std::string str_card_ids_to_delete = "";

        for (auto & element : v_cards_to_delete_)
            str_card_ids_to_delete.append("'" + element + "', ");
        str_card_ids_to_delete.pop_back();
        str_card_ids_to_delete.pop_back();

        pg_query->clear();
        const QString qstr_card_ids_to_delete = QString::fromStdString ("DELETE FROM issues WHERE idCard IN (" + str_card_ids_to_delete + ")" );
        c_out << qstr_card_ids_to_delete.toStdString() << std::endl;
        colour_out("Карточки, закрытые на доске, были удалены");
        pg_query->exec(qstr_card_ids_to_delete);

        /* Очищаем список карточек к удалению - быстро swap'ом */
        clear_stl_container(v_cards_to_delete_);

    }
    pgDatabase.commit();
}

void list_checker::initialize_tracking_apis()
{
    c_out << "ListChecker: Initializing APIs: Trello, Jira..." << std::endl;

    p_trello = new trello_api(sett["trello_key"], sett["trello_token"], sett["trello_prefix"]);
    p_jira   = new jira_api(sett["jira_token"], sett["jira_base_url"], tg_bot.getApi(), sett["support_TgID"], pgDatabase, sett["script_path_mail_sender"]);
}

void list_checker::nix_bg_worker(QElapsedTimer* timer)
{
    auto listchecker_mem_size = get_mem_size(sett["script_path_getmem_listchecker"]);
    std::string uptime = itoa(timer->elapsed()/1000/60);
    tg_bot.getApi().sendMessage(sett["support_TgID"], "ListChecker " + itoa(listchecker_mem_size) + " Мб в памяти" + ", uptime " + uptime + " min");

/*  tgBot.getApi().sendMessage(supportTgID, "Ответ от ListChecker: число байт = 32, время < 1мс, TTL=124"); */

}

void list_checker::service_owner_changed(const QString &name, const QString &old_owner, const QString &new_owner)
{
    Q_UNUSED(old_owner);
    if (name == "chanserv.qdbus")
    {
        c_out << "DBus: Got signal 'ServiceOwnerChanged'. Is chanserv exited?\n" << name.toStdString() << old_owner.toStdString() << new_owner.toStdString() << std::endl;
        if (!new_owner.isEmpty())
        {
            // New owner in town
            // emit Initialized();
            // or if you control the interface and both sides, you can wait for
            // a "Ready()" signal before declaring FooService ready for business.
        }
        else
        {
            // indicate we've lost connection, etc
            // emit Uninitialized();
        }
    }
}
