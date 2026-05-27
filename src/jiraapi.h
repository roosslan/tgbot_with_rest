#pragma once

#include "structures.h"
#include "support.h"

#include <QUrlQuery>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <qnetworkaccessmanager.h>
#include <qthread.h>
#include <qsqlquery.h>
#include <QEventLoop>
#include <QNetworkReply>
#include <QHttpPart>
#include <qfileinfo.h>

class jira_api
{
    std::string m_jira_token_;
    QString     m_base_url_;
    long        m_support_tg_id_;
    const TgBot::Api* tg_bot_api_;
    QSqlDatabase& m_pg_database_;
    std::string m_mail_sender_path_;

private slots:
    void onFinished();

public:
    jira_api(const std::string& token, const std::string& base_url, const TgBot::Api &api, const std::string& support_tg_id, const QSqlDatabase& pgsql_db, const std::string& mail_script_path);
    ~jira_api();

    static size_t curl_write_callback(const char* buff_ptr, size_t sz, size_t cnt_memb, void* userdata);
    int create_task(user_dispatcher* ud, const std::string& project_name);
    bool add_attach(const std::string& file_path, const std::string& issue_key);
    int get_task_info(const std::string& issue_key);
    int change_task_status(const std::string& issue_key, const int new_status);
};
