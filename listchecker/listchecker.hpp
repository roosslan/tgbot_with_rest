#ifndef LISTCHECKER_H
#define LISTCHECKER_H

#include <QtSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QHostInfo>
#include <QtDBus/QDBusInterface>
#include "boost/algorithm/string/trim.hpp"
#include "clientdbus.hpp"
#include "../src/jiraapi.h"
#include "../src/abstract_chanserv.h"
#include "../src/trelloapi.h"

#include <string>
#include <chrono>
#include <syslog.h>

class list_checker final : public AbstractChanserv
{
    std::vector<std::string> v_id_cards_;
    std::vector<std::string> v_card_ids_to_delete_;
    std::vector<std::string> v_cards_to_delete_;
    InlineKeyboardMarkup::Ptr kb_go_dashboard_;

    InlineKeyboardButton::Ptr btn_go_dashboard_;

    /* Создание кнопок, инлайновой клавы */
    void bootstrap();

    /* Блок для уменьшения разросшейся ф-ции CheckTheList() */
    void process_every_card(QSqlQuery* pg_query);

    /* Добавляем исполнителя в таблице БД "issues" */
    void assign_contractor_to_card(const std::string& sql_query, const std::string& card_id, contractor* p_contractor);

    /* Обработка карточки, если она перемещена в колонку 'Задачу удалить. Заказчик, пересоздай!' */
    void rework_card_and_notify(const std::string& card_id, int card_index);

    /* Если карточку вернули обратно в первую колонку, то ее надо очистить от исполнителя */
    std::string sync_distributed_card(const std::string card_id);

    /* Функции проверки карточек на доске и БД. В случае различий - выполнение соотв.действий (sync, оичстка или удаление) */
    void sync_n_manage_contractors_and_cards(const std::string card_id, std::string trello_card_list_id, int card_index);

    /* Подготовка к переезду на PostgreSQL */
    void checks_bootstrap(QElapsedTimer* timer);
    void delete_cards(QSqlQuery* pg_query);

public:
    explicit list_checker();
    ~list_checker();
    list_checker& operator=(list_checker const&) = delete;
    list_checker& operator=(const list_checker&&) = delete;
    bool operator()(std::string t) = delete;

    Bot tg_bot;

    client_dbus::Ptr bot_dbus;
    trello_api*  p_trello;
    jira_api* p_jira;

    void check_the_list(QElapsedTimer* timer);

    /* Переподключение в случае возникновения ошибки [Microsoft][ODBC Driver 18 for SQL Server]The connection is broken and recovery is not possible.
     * The connection is marked by the client driver as unrecoverable. No attempt was made to restore the connection.
     * void ReconnectDB(std::string firstTime = "");
     */

    void initialize_tracking_apis();

    void nix_bg_worker(QElapsedTimer* timer);
private slots:
    /* DBus */
    void service_owner_changed(const QString &name, const QString& old_owner, const QString& new_owner);
};

#endif // LISTCHECKER_H
