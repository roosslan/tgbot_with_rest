#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <QCoreApplication>
#include "src/qtests.h"
#include "support.h"
#include "chanserv.h"

#include <string>
#include <QCryptographicHash>
#include <QDateTime>
#include <QByteArray>
#include <QString>
#include <QDebug>
#include <QNetworkInterface>
#include <QNetworkProxy>
#include <QUrlQuery>
#include <atomic>
#include <cmath>
#include <memory>
#include <thread>

/* Сравнение токена вебхука за время, не зависящее от совпавшего префикса */
static bool secure_equals(const QByteArray &received, const std::string &expected)
{
    if (received.size() != static_cast<qsizetype>(expected.size()))
        return false;

    unsigned char diff = 0;
    for (qsizetype i = 0; i < received.size(); ++i)
        diff |= static_cast<unsigned char>(received[i]) ^ static_cast<unsigned char>(expected[i]);
    return diff == 0;
}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

/*  QTest::qExec(new TestMinimal, argc, argv);  */

    /* Сообщаем аптайм программы для диагностических целей */
    QElapsedTimer* upTimer = new QElapsedTimer();
    upTimer->start();

    /* Handler, чтобы qDebug() писал свой вывод в файл */
    qInstallMessageHandler(bg_message_handler);

    /* Ошибки чтения конфига и подключения к БД приходят исключениями из конструктора и IInitializeDB() */
    std::unique_ptr<chanserv_bot> bot_holder;
    try
    {
        bot_holder = std::make_unique<chanserv_bot>();

        // Открываем БД микрософт SQL Server
        bot_holder->IInitializeDB();
    }
    catch (const std::exception &e)
    {
        c_out << "Ошибка инициализации бота: " << e.what() << std::endl;
        return 1;
    }
    chanserv_bot& jira_bot = *bot_holder;

    jira_bot.initialize_tracking_apis();

    c_out << "Creating keyboards..." << std::endl;
    jira_bot.create_keyboards();

    jira_bot.initialize_commands();

    if (argc > 1){
        if (argv[1] == std::string("notify"))
        {
            c_out << "Режим рассылки. Будет разослано сообщение \"" + std::string(argv[2]) + "\"" << std::endl; /* "\"\nНажмите [Y] (большую) и клавишу Enter для продолжения" << std::endl; */
            jira_bot.service_notification(std::string(argv[2]));
        }
    }
    else
    {
        /* Бесконечный служебный цикл, отчет о памяти, перезапуск listchecker etc */
        c_out << "Профилирование будет повторяться каждые 4 часа" << std::endl;
        active_object monitor_mem_size_worker([&jira_bot, upTimer]
        {
            QEventLoop loop;
            jira_bot.qt_bg_worker(upTimer);
            QTimer::singleShot(14400000, &loop, SLOT(quit()));
            loop.exec();
        });

        /* Обработчик вебхуков Jira */
        c_out << "Webhook HttpServer for Jira listening port " << jira_bot.sett["port_jira_webhook"] << " on" << std::endl;
        const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
        for (const QHostAddress &address: QNetworkInterface::allAddresses()){
            if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
                c_out << address.toString().toStdString() << std::endl;
        }

        /* Сервер вебхуков создаётся один раз и работает до завершения бота.
         * Jira должна вызывать адрес вида http://host:port/UPD/${issue.key}?token=<jira.webhook_secret> */
        const std::string webhook_secret = jira_bot.sett["jira_webhook_secret"];
        std::atomic<QEventLoop*> jira_listener_loop{nullptr};

        std::thread jira_listener([&jira_bot, &jira_listener_loop, webhook_secret]{
            if (webhook_secret.empty()) {
                c_out << "jira.webhook_secret в chanserv.toml не задан, вебхуки Jira отключены" << std::endl;
                return;
            }

            QEventLoop loop;

            QHttpServer server;
            server.route("/", [](const QHttpServerRequest &request) {
                c_out << request.url().path().toStdString() << std::endl;
                return "";
            });

            server.setMissingHandler(
                &server, [&jira_bot, webhook_secret](const QHttpServerRequest &request, QHttpServerResponder &responder) {
                    const QByteArray token = QUrlQuery(request.url()).queryItemValue("token").toUtf8();
                    if (!secure_equals(token, webhook_secret)) {
                        c_out << "Вебхук Jira отклонён (неверный token) от " << request.remoteAddress().toString().toStdString() << std::endl;
                        responder.sendResponse(QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden));
                        return;
                    }

                    c_out << "Received a trigger from Jira: " << request.url().path().toStdString() << ", passed to JiraWebhooksHandler()" << std::endl;

                    /* При загрузке изображений во вновь созданный тикет вызывается вебхук UPD/
                     * и boost::json'у приходится одновременно обрабатывать дважды одно и то же
                     */
                    sleep(2);

                    jira_bot.jira_webhooks_handler(request.url().path().toStdString());
                    responder.sendResponse(QHttpServerResponse("text/plain", QByteArray("Pass to handler"), QHttpServerResponse::StatusCode::Ok));
                });

            QTcpServer tcpServer;
            if (!tcpServer.listen(QHostAddress::Any, atol(jira_bot.sett["port_jira_webhook"].c_str()))){
                c_out << "Failed to listen on port " << jira_bot.sett["port_jira_webhook"] << ": " << tcpServer.errorString().toStdString() << std::endl;
                return;
            }

            if (!server.bind(&tcpServer)) {
                c_out << "Failed to bind QHttpServer to QTcpServer" << std::endl;
                return;
            }

            jira_listener_loop = &loop;
            loop.exec();
            jira_listener_loop = nullptr;
        });

        try {
            jira_bot.bot_start(&app);
        }
        catch (std::exception &e){
            c_out << "BotStart's error: " + std::string(e.what()) << std::endl;
        }

        send_msg_to_support(jira_bot.m_tg_bot.getApi(), jira_bot.sett["support_TgID"], "⚠️ Бот работу завершил.");

        /* Останавливаем цикл событий сервера вебхуков из его же потока */
        if (QEventLoop* loop = jira_listener_loop.load())
            QMetaObject::invokeMethod(loop, "quit", Qt::QueuedConnection);
        jira_listener.join();
        monitor_mem_size_worker.terminate();
        raise(SIGINT);

      } /* блок else */

    c_out << "Бот свою работу завершает. Сау булыгыз!" << std::endl;

    return 1;
}


