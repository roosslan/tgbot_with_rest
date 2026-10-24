#include "chanconf.h"

#include <stdexcept>

ChanConf::ChanConf()
{
    c_out << "Configuration class starting..." << std::endl;

    IInitializeConfig();

    /* pgSQL settings */
    const std::vector<std::string> pg_sql_settings = {"pghost", "pgdb", "pguser", "pgpwd"};
    foreach (const auto key, pg_sql_settings) {
        pg[key] = QString::fromStdString(conf_value("main", key));
    }
    pg["pgport"] = QString::fromStdString(conf_value("main", "pgport", "5432"));

    IInitializeDB();

    read_settings_from_db();

    /* Настройки, которые хранятся не в БД, а в chanserv.toml */
    sett["jira_browse_url"]     = conf_value("jira", "browse_url");
    sett["jira_webhook_secret"] = conf_value("jira", "webhook_secret");
    sett["session_ttl_hours"]   = conf_value("main", "session_ttl_hours", "24");
    sett["utc_offset_hours"]    = conf_value("main", "utc_offset_hours", "3");
    sett["sticker_no_tasks"]    = conf_value("stickers", "no_tasks");
}

bool ChanConf::is_valid() const
{
    return isValid;
}

void ChanConf::IInitializeConfig()
{
    c_out << "ChanConf's config " << config_file_ << std::endl;

    try
    {
        chanserv_toml_ = toml::parse_file(config_file_);
    }
    catch (const toml::parse_error& e)
    {
        isValid = false;
        c_out << "ChanConf::IInitializeConfig: Cannot parse this configuration file: " << config_file_ << " (" << std::string(e.description()) << ")"
              << " Please make sure, that configuration file is set in env.variable CHANSERV_CONFIG" << std::endl;
    }
}

std::string ChanConf::conf_value(std::string_view section, std::string_view key, const std::string& default_value) const
{
    return toml_value_as_string(chanserv_toml_, section, key, default_value);
}

std::string toml_value_as_string(const toml::table& tbl, std::string_view section, std::string_view key, const std::string& default_value)
{
    const auto node = tbl[section][key];
    if (const auto str = node.value_exact<std::string>())
        return *str;
    if (const auto num = node.value_exact<int64_t>())
        return std::to_string(*num);
    return default_value;
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
        const std::string err = pgDatabase.lastError().text().toStdString();
        c_out << err << std::endl;
        throw std::runtime_error("PostgreSQL: " + err);
    }

    return 0;
}

std::string ChanConf::get_setting(const std::string& field_name)
{
    QSqlQuery pg_query(pgDatabase);
    pg_query.prepare("SELECT value FROM settings WHERE name = ?");
    pg_query.addBindValue(QString::fromStdString(field_name));
    if (!pg_query.exec() || !pg_query.next())
        return "";
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
        "tekla_admin_tgid",
        "tg_bot_token",
        "trello_token",
        "trello_key",
        "trello_prefix"    };

    foreach (const auto key, chanserv_settings)
    {
        sett[key] = get_setting(key.c_str());
    }
}
