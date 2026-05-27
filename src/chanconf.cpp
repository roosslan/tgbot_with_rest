#include "chanconf.h"

ChanConf::ChanConf()
{
    c_out << "Configuration class starting..." << std::endl;

    IInitializeConfig();

    /* pgSQL settings */
    const std::vector<std::string> pg_sql_settings = {"pghost", "pgdb", "pguser", "pgpwd", "pgport"};
    foreach (const auto key, pg_sql_settings) {
        pg[key] = chanserv_ini_.GetValue("main", key.c_str(), key.c_str());
    }

    IInitializeDB();

    read_settings_from_db();
}

bool ChanConf::is_valid() const
{
    return isValid;
}

void ChanConf::IInitializeConfig()
{
    c_out << "ChanConf's config " << config_file_ << std::endl;
    chanserv_ini_.SetUnicode();

    SI_Error rc =chanserv_ini_.LoadFile(config_file_.c_str());
    if (rc != 0)
    {
        isValid = false;
        c_out << "ChanConf::InitializeINIconfig: Cannot open this configuration file: " << config_file_ << " Please make sure, that configuration file is set in env.variable CHANSERV_CONFIG" << std::endl;
    }
}

int ChanConf::IInitializeDB()
{
    c_out << "ChanConf: Connecting to PostgreSQL DB " + pg["pghost"].toStdString() + "\\" + pg["pgdb"].toStdString() + " as user " + pg["pguser"].toStdString() << std::endl;
    pgDatabase.setConnectOptions();
    pgDatabase.setHostName(pg["pghost"]);
    pgDatabase.setUserName(pg["pguser"]);
    pgDatabase.setPassword(pg["pgpwd"]);
    pgDatabase.setDatabaseName(pg["pgdb"]);
    pgDatabase.setPort(pg["pgport"].toInt());

    const bool rres = pgDatabase.open();
    if (!rres)
    {
        isValid = false;
        c_out << pgDatabase.lastError().text().toStdString() << std::endl;
        throw;
    }

    return 0;
}

std::string ChanConf::get_setting(const std::string& field_name)
{
    QSqlQuery pg_query(pgDatabase);
    pg_query.exec("SELECT value FROM settings WHERE name = '" + QString::fromStdString(field_name) + "'");
    pg_query.next();
    return pg_query.value(0).toString().toStdString();
}
void ChanConf::read_settings_from_db()
{
    c_out << "ChanConf: Start SELECTing settings from the database" << std::endl;

    const std::vector<std::string> chanserv_settings =
       {
        "board_link",
        "distribution_list",
        "fam_otp_key",
        "jira_base_url",
        "jira_browse_url_prefix",
        "jira_project_url",
        "jira_token",
        "port_jira_webhook",
        "repeat_list_check_in_sec",
        "rework_list",
        "script_path_getmem_chanserv",
        "script_path_getmem_listchecker",
        "script_path_listchecker",
        "script_path_mail_sender",
        "script_path_reboot",
        "support_TgID",
        "tg_bot_token",
        "trello_token",
        "trello_key",
        "trello_prefix"    };

    foreach (const auto key, chanserv_settings)
    {
        sett[key] = get_setting(key.c_str());
    }
}
