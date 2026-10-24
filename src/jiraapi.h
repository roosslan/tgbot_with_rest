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

#include <algorithm>
#include <cctype>
#include <unordered_map>

/* Запоминает исполнителя задачи и возвращает true, если ему ещё не отправлялось письмо по этой задаче
 * (задача новая или исполнитель сменился). Вебхук UPD приходит на любое изменение задачи, письмо - только на назначение */
inline bool remember_assignee(std::unordered_map<std::string, std::string>& notified, const std::string& issue_key, std::string email)
{
    std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) { return std::tolower(c); });
    if (email.empty())
    {
        notified.erase(issue_key);    /* исполнителя сняли - при следующем назначении письмо уйдёт снова */
        return false;
    }

    auto [it, inserted] = notified.try_emplace(issue_key, email);
    if (!inserted && it->second == email)
        return false;
    it->second = email;
    return true;
}

class jira_api
{
    std::string m_jira_token_;
    QString     m_base_url_;
    std::string m_browse_url_;   /* Префикс ссылки на задачу, например https://jira.local:8443/browse/ */
    long        m_support_tg_id_;
    const TgBot::Api* tg_bot_api_;
    QSqlDatabase& m_pg_database_;
    std::string m_mail_sender_path_;
    /* Кому уже отправлено письмо о задаче: ключ задачи -> e-mail исполнителя */
    std::unordered_map<std::string, std::string> m_notified_assignee_;

private slots:
    void onFinished();

public:
    jira_api(const std::string& token, const std::string& base_url, const std::string& browse_url, const TgBot::Api &api, const std::string& support_tg_id, const QSqlDatabase& pgsql_db, const std::string& mail_script_path);
    ~jira_api();

    static size_t curl_write_callback(const char* buff_ptr, size_t sz, size_t cnt_memb, void* userdata);
    int create_task(user_dispatcher* ud, const std::string& project_name);
    bool add_attach(const std::string& file_path, const std::string& issue_key);
    int get_task_info(const std::string& issue_key);
    int change_task_status(const std::string& issue_key, const int new_status);
};
