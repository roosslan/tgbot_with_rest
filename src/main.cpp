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
#include <cmath>


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

/*  QTest::qExec(new TestMinimal, argc, argv);  */

    /* Сообщаем аптайм программы для диагностических целей */
    QElapsedTimer* upTimer = new QElapsedTimer();
    upTimer->start();

    /* Handler, чтобы qDebug() писал свой вывод в файл */
    qInstallMessageHandler(bg_message_handler);

    chanserv_bot jira_bot;

    // Открываем БД микрософт SQL Server
    jira_bot.IInitializeDB();

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

        active_object jira_listener([&jira_bot]{
            QEventLoop loop;

            QHttpServer server;
            server.route("/", [](const QHttpServerRequest &request) {
                c_out << request.url().toString().toStdString() << std::endl;
                return "";
            });

            server.setMissingHandler(
                &server, [&jira_bot](const QHttpServerRequest &request, QHttpServerResponder &responder) {
                    c_out << "Received a trigger from Jira: " << request.url().path().toStdString() << ", passed to JiraWebhooksHandler()" << std::endl;

                    /* При загрузке изображений во вновь созданный тикет вызывается вебхук UPD/
                     * и boost::json'у приходится одновременно обрабатывать дважды одно и то же
                     */
                    sleep(2);

                    jira_bot.jira_webhooks_handler(request.url().path().toStdString());
                    return QHttpServerResponse("text/plain", QByteArray("Pass to handler"), QHttpServerResponse::StatusCode::NotFound);
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

            QTimer::singleShot(144000, &loop, SLOT(quit()));
            loop.exec();
        });

        try {
            jira_bot.bot_start(&app);
        }
        catch (std::exception &e){
            c_out << "BotStart's error: " + std::string(e.what()) << std::endl;
        }

        send_msg_to_support(jira_bot.m_tg_bot.getApi(), jira_bot.sett["support_TgID"], "⚠️ Бот работу завершил.", *jira_bot.uD, nullptr);

        jira_listener.terminate();
        monitor_mem_size_worker.terminate();
        raise(SIGINT);

      } /* блок else */

    c_out << "Бот свою работу завершает. Сау булыгыз!" << std::endl;

    return 1;
}


