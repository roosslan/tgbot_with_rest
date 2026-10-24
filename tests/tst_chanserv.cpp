/* Модульные тесты chanserv: логика, не требующая БД, Telegram, Trello и Jira */

#include <QtTest>

#include "access.h"
#include "chanconf.h"
#include "session_store.h"
#include "support.h"
#include "task_queue.h"

#include <thread>

using namespace std::chrono_literals;

class tst_chanserv : public QObject
{
    Q_OBJECT

private slots:
    /* access.h */
    void callback_roles_data();
    void callback_roles();
    void step_roles();
    void role_permissions();

    /* session_store.h */
    void session_create_and_find();
    void session_eviction();
    void session_activity_prolongs_life();

    /* Вебхук Jira */
    void secure_equals_cases();
    void webhook_token_data();
    void webhook_token();

    /* TOTP мастер-ключа Revit */
    void totp_rfc6238_data();
    void totp_rfc6238();
    void totp_default_digits();

    /* Экранирование и обрезка текста от юзера */
    void escape_all_does_not_double_quotes();
    void escape_all_json_and_section();
    void truncate_data();
    void truncate();
    void itoa_values();

    /* chanserv.toml */
    void toml_values();
    void sample_config();

    /* Очередь задач главного потока */
    void task_queue_order();
    void task_queue_posted_during_run();
    void task_queue_from_threads();
};

/* ===================================================================================== access.h */

void tst_chanserv::callback_roles_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<int>("role");

    const int dev = static_cast<int>(required_role::developer);
    const int staff = static_cast<int>(required_role::staff);
    const int anyone = static_cast<int>(required_role::anyone);

    QTest::newRow("run_command") << "run_command" << dev;
    QTest::newRow("msg_as_bot") << "msg_as_bot" << dev;
    QTest::newRow("msg_to_gpt") << "msg_to_gpt" << dev;
    QTest::newRow("reboot_host") << "reboot_host" << dev;
    QTest::newRow("send_file") << "send_file" << dev;
    QTest::newRow("add_new_contractor") << "add_new_contractor" << staff;
    QTest::newRow("set_new_msi_version") << "set_new_msi_version" << staff;
    QTest::newRow("approve") << "approve_key_request_123456" << staff;
    QTest::newRow("reject") << "_reject_key_request_123456" << staff;
    QTest::newRow("new_task") << "new_task" << anyone;
    QTest::newRow("my_tasks") << "my_tasks" << anyone;
    QTest::newRow("req_master_key") << "req_master_key" << anyone;
    QTest::newRow("prefix is not enough") << "run_command_x" << anyone;
    QTest::newRow("empty") << "" << anyone;
}

void tst_chanserv::callback_roles()
{
    QFETCH(QString, data);
    QFETCH(int, role);
    QCOMPARE(static_cast<int>(role_for_callback(data.toStdString())), role);
}

void tst_chanserv::step_roles()
{
    QVERIFY(role_for_step(developer_run_host_command) == required_role::developer);
    QVERIFY(role_for_step(developer_send_msg_as_bot) == required_role::developer);
    QVERIFY(role_for_step(developer_send_msg_to_gpt) == required_role::developer);
    QVERIFY(role_for_step(staff_add_new_contractor) == required_role::staff);
    QVERIFY(role_for_step(staff_set_new_msi_version) == required_role::staff);
    QVERIFY(role_for_step(task_section_step1) == required_role::anyone);
    QVERIFY(role_for_step(revit_master_key_reason_step1) == required_role::anyone);
    QVERIFY(role_for_step(0) == required_role::anyone);
}

void tst_chanserv::role_permissions()
{
    /* is_allowed(role, is_staff, is_developer) */
    QVERIFY(is_allowed(required_role::anyone, false, false));

    QVERIFY(!is_allowed(required_role::staff, false, false));
    QVERIFY(is_allowed(required_role::staff, true, false));
    QVERIFY(is_allowed(required_role::staff, false, true));

    QVERIFY(!is_allowed(required_role::developer, false, false));
    QVERIFY(!is_allowed(required_role::developer, true, false));
    QVERIFY(is_allowed(required_role::developer, false, true));
}

/* ================================================================================ session_store.h */

void tst_chanserv::session_create_and_find()
{
    session_store store;
    const auto t0 = session_store::clock::time_point{} + 1h;

    QVERIFY(store.find(42, t0) == nullptr);

    user_dispatcher* ud = store.create(42, "ivanov", "Иван", t0);
    QVERIFY(ud != nullptr);
    QCOMPARE(ud->chat_user_tg_id, 42L);
    QCOMPARE(ud->chat_user_tg_name, std::string("ivanov"));
    QCOMPARE(ud->chat_user_first_name, std::string("Иван"));
    QCOMPARE(ud->jira_task.stat_, 0);
    QCOMPARE(store.size(), size_t(1));

    /* Повторное создание возвращает ту же сессию */
    QCOMPARE(store.create(42, "other", "Other", t0), ud);
    QCOMPARE(store.find(42, t0), ud);
    QCOMPARE(store.size(), size_t(1));
}

void tst_chanserv::session_eviction()
{
    session_store store;
    const auto t0 = session_store::clock::time_point{} + 1h;

    store.create(1, "a", "A", t0);
    store.create(2, "b", "B", t0 + 10h);

    QCOMPARE(store.evict_idle(t0 + 24h, 24h), size_t(0));   /* ровно ttl - ещё живёт */
    QCOMPARE(store.evict_idle(t0 + 25h, 24h), size_t(1));
    QVERIFY(store.find(1, t0 + 25h) == nullptr);
    QVERIFY(store.find(2, t0 + 25h) != nullptr);
    QCOMPARE(store.size(), size_t(1));
}

void tst_chanserv::session_activity_prolongs_life()
{
    session_store store;
    const auto t0 = session_store::clock::time_point{} + 1h;

    store.create(7, "c", "C", t0);
    QVERIFY(store.find(7, t0 + 20h) != nullptr);                 /* юзер снова написал */
    QCOMPARE(store.evict_idle(t0 + 30h, 24h), size_t(0));
    QCOMPARE(store.evict_idle(t0 + 45h, 24h), size_t(1));
}

/* ==================================================================================== Вебхук Jira */

void tst_chanserv::secure_equals_cases()
{
    QVERIFY(secure_equals("s3cr3t", "s3cr3t"));
    QVERIFY(!secure_equals("s3cr3T", "s3cr3t"));
    QVERIFY(!secure_equals("s3cr3", "s3cr3t"));
    QVERIFY(!secure_equals("s3cr3tt", "s3cr3t"));
    QVERIFY(!secure_equals("", "s3cr3t"));
    QVERIFY(secure_equals("", ""));
}

void tst_chanserv::webhook_token_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("secret");
    QTest::addColumn<bool>("valid");

    QTest::newRow("valid") << "http://h:1/UPD/TEKLA-1?token=abc123" << "abc123" << true;
    QTest::newRow("with other params") << "http://h:1/UPD/TEKLA-1?user=x&token=abc123" << "abc123" << true;
    QTest::newRow("url-encoded") << "http://h:1/UPD/TEKLA-1?token=a%2Bb%26c" << "a+b&c" << true;
    QTest::newRow("wrong") << "http://h:1/UPD/TEKLA-1?token=abc124" << "abc123" << false;
    QTest::newRow("missing") << "http://h:1/UPD/TEKLA-1" << "abc123" << false;
    QTest::newRow("empty token") << "http://h:1/UPD/TEKLA-1?token=" << "abc123" << false;
    QTest::newRow("empty secret") << "http://h:1/UPD/TEKLA-1?token=" << "" << false;
    QTest::newRow("token in path") << "http://h:1/UPD/abc123" << "abc123" << false;
}

void tst_chanserv::webhook_token()
{
    QFETCH(QString, url);
    QFETCH(QString, secret);
    QFETCH(bool, valid);
    QCOMPARE(webhook_token_valid(QUrl(url), secret.toStdString()), valid);
}

/* ======================================================================================== TOTP */

void tst_chanserv::totp_rfc6238_data()
{
    /* Тестовые векторы RFC 6238, приложение B (SHA1, шаг 30 с, 8 цифр) */
    QTest::addColumn<qint64>("time");
    QTest::addColumn<QString>("expected");

    QTest::newRow("59") << qint64(59) << "94287082";
    QTest::newRow("1111111109") << qint64(1111111109) << "07081804";
    QTest::newRow("1111111111") << qint64(1111111111) << "14050471";
    QTest::newRow("1234567890") << qint64(1234567890) << "89005924";
    QTest::newRow("2000000000") << qint64(2000000000) << "69279037";
    QTest::newRow("20000000000") << qint64(20000000000) << "65353130";
}

void tst_chanserv::totp_rfc6238()
{
    QFETCH(qint64, time);
    QFETCH(QString, expected);
    QCOMPARE(QString::fromStdString(generate_totp("12345678901234567890", time, 8, 30)), expected);
}

void tst_chanserv::totp_default_digits()
{
    const std::string key = generate_revit_master_key("any-secret");
    QCOMPARE(key.size(), size_t(6));
    QVERIFY(std::all_of(key.begin(), key.end(), [](char c) { return c >= '0' && c <= '9'; }));
}

/* ========================================================================= Экранирование, обрезка */

void tst_chanserv::escape_all_does_not_double_quotes()
{
    /* Значения в SQL идут через bind, поэтому ' больше не удваивается */
    user_dispatcher ud{};
    ud.jira_task.description = "O'Brien's model";
    ud.jira_task.subj = "it's";
    escape_all(ud);
    QCOMPARE(ud.jira_task.description, std::string("O'Brien's model"));
    QCOMPARE(ud.jira_task.subj, std::string("it's"));
}

void tst_chanserv::escape_all_json_and_section()
{
    user_dispatcher ud{};
    ud.jira_task.description = "строка 1\nстрока \"2\"\t\\";
    ud.jira_task.section = "АР КЖ ОВ";
    escape_all(ud);
    QCOMPARE(ud.jira_task.description, std::string("строка 1\\nстрока \\\"2\\\"\\t\\\\"));
    /* Внутри labels Jira пробелы запрещены */
    QCOMPARE(ud.jira_task.section, std::string("АРКЖОВ"));
}

void tst_chanserv::truncate_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("width");
    QTest::addColumn<QString>("expected");

    QTest::newRow("short") << "abc" << 5 << "abc";
    QTest::newRow("exact") << "abcde" << 5 << "abcde";
    QTest::newRow("long") << "abcdefgh" << 5 << "abcde...";
    /* 6 символов кириллицы = 12 байт UTF-8, в лимит 6 символов укладываются */
    QTest::newRow("cyrillic fits") << "привет" << 6 << "привет";
    QTest::newRow("cyrillic long") << "привет мир" << 6 << "привет...";
}

void tst_chanserv::truncate()
{
    QFETCH(QString, text);
    QFETCH(int, width);
    QFETCH(QString, expected);
    QCOMPARE(QString::fromStdString(truncate_msg_from_user(text.toStdString(), width)), expected);
}

void tst_chanserv::itoa_values()
{
    QCOMPARE(itoa(0), std::string("0"));
    QCOMPARE(itoa(-17), std::string("-17"));
    QCOMPARE(itoa(9275911689L), std::string("9275911689"));
}

/* =============================================================================== chanserv.toml */

void tst_chanserv::toml_values()
{
    const toml::table tbl = toml::parse(R"(
        [main]
        host = "10.0.0.1"
        port = 5432
        flag = true
        [jira]
        browse_url = "https://jira.local/browse/"
    )");

    QCOMPARE(toml_value_as_string(tbl, "main", "host"), std::string("10.0.0.1"));
    QCOMPARE(toml_value_as_string(tbl, "main", "port"), std::string("5432"));
    QCOMPARE(toml_value_as_string(tbl, "main", "flag", "def"), std::string("def"));      /* неподдерживаемый тип */
    QCOMPARE(toml_value_as_string(tbl, "main", "missing", "def"), std::string("def"));
    QCOMPARE(toml_value_as_string(tbl, "nosection", "host", "def"), std::string("def"));
    QCOMPARE(toml_value_as_string(tbl, "jira", "browse_url"), std::string("https://jira.local/browse/"));
}

void tst_chanserv::sample_config()
{
    /* Пример конфига из docker-img должен разбираться и содержать все ключи, которые читает ChanConf */
    toml::table tbl;
    try {
        tbl = toml::parse_file(CHANSERV_SAMPLE_CONFIG);
    } catch (const toml::parse_error& e) {
        QFAIL(e.what());
    }

    for (const char* key : {"pghost", "pgport", "pgdb", "pguser", "pgpwd", "session_ttl_hours"})
        QVERIFY2(tbl["main"][key], key);

    QVERIFY(!toml_value_as_string(tbl, "jira", "browse_url").empty());
    QVERIFY(toml_value_as_string(tbl, "jira", "browse_url").ends_with("/"));
    QVERIFY(tbl["jira"]["webhook_secret"].is_string());
    QCOMPARE(toml_value_as_string(tbl, "main", "pgport"), std::string("5432"));
}

/* ================================================================================= task_queue.h */

void tst_chanserv::task_queue_order()
{
    task_queue q;
    std::vector<int> order;
    q.post([&] { order.push_back(1); });
    q.post([&] { order.push_back(2); });
    q.post([&] { order.push_back(3); });

    QCOMPARE(q.size(), size_t(3));
    QCOMPARE(q.run_pending(), size_t(3));
    QCOMPARE(order, (std::vector<int>{1, 2, 3}));
    QCOMPARE(q.size(), size_t(0));
    QCOMPARE(q.run_pending(), size_t(0));
}

void tst_chanserv::task_queue_posted_during_run()
{
    /* Задача, поставленная во время выполнения, ждёт следующего вызова run_pending() */
    task_queue q;
    int runs = 0;
    q.post([&] { ++runs; q.post([&] { ++runs; }); });

    QCOMPARE(q.run_pending(), size_t(1));
    QCOMPARE(runs, 1);
    QCOMPARE(q.run_pending(), size_t(1));
    QCOMPARE(runs, 2);
}

void tst_chanserv::task_queue_from_threads()
{
    task_queue q;
    std::atomic<int> sum{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t)
        threads.emplace_back([&] { for (int i = 0; i < 250; ++i) q.post([&] { sum += 1; }); });
    for (auto& th : threads)
        th.join();

    QCOMPARE(q.run_pending(), size_t(1000));
    QCOMPARE(sum.load(), 1000);
}
QTEST_GUILESS_MAIN(tst_chanserv)
#include "tst_chanserv.moc"
