#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <QCoreApplication>
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
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    /* SIGINT/SIGTERM только выставляют флаг, бот завершает работу сам: long poll, вебхуки и мониторинг останавливаются */
    std::signal(SIGINT, [](int) { request_stop(); });
    std::signal(SIGTERM, [](int) { request_stop(); });

    /* Сообщаем аптайм программы для диагностических целей */
    QElapsedTimer up_timer;
    up_timer.start();

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

    if (argc > 1)
    {
        if (argv[1] == std::string("notify") && argc > 2)
        {
            c_out << "Режим рассылки. Будет разослано сообщение \"" + std::string(argv[2]) + "\"" << std::endl;
            jira_bot.service_notification(std::string(argv[2]));
        }
        return 0;
    }

    /* Служебный поток: раз в 4 часа ставит в очередь главного потока отчёт о памяти и проверку listchecker'а.
     * Сам Telegram API и БД из этого потока не используются */
    std::mutex stop_mutex;
    std::condition_variable stop_cv;

    c_out << "Профилирование будет повторяться каждые 4 часа" << std::endl;
    std::thread monitor_mem_size_worker([&jira_bot, &up_timer, &stop_mutex, &stop_cv]
    {
        std::unique_lock lock(stop_mutex);
        do {
            jira_bot.post_task([&jira_bot, &up_timer] { jira_bot.qt_bg_worker(&up_timer); });
        } while (!stop_cv.wait_for(lock, std::chrono::hours(4), [] { return stop_requested(); }));
    });

    /* Обработчик вебхуков Jira */
    c_out << "Webhook HttpServer for Jira listening port " << jira_bot.sett["port_jira_webhook"] << " on" << std::endl;
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address: QNetworkInterface::allAddresses()){
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
            c_out << address.toString().toStdString() << std::endl;
    }

    /* Сервер вебхуков создаётся один раз и работает до завершения бота.
     * Jira должна вызывать адрес вида http://host:port/UPD/${issue.key}?token=<jira.webhook_secret>
     * Вебхук только ставит задачу в очередь главного потока, поэтому вебхуки обрабатываются строго по очереди */
    const std::string webhook_secret = jira_bot.sett["jira_webhook_secret"];
    const quint16 webhook_port = static_cast<quint16>(atol(jira_bot.sett["port_jira_webhook"].c_str()));

    std::thread jira_listener([&jira_bot, webhook_secret, webhook_port]{
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
                if (!webhook_token_valid(request.url(), webhook_secret)) {
                    c_out << "Вебхук Jira отклонён (неверный token) от " << request.remoteAddress().toString().toStdString() << std::endl;
                    responder.sendResponse(QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden));
                    return;
                }

                const std::string path = request.url().path().toStdString();
                c_out << "Received a trigger from Jira: " << path << ", queued for JiraWebhooksHandler()" << std::endl;
                jira_bot.post_task([&jira_bot, path] { jira_bot.jira_webhooks_handler(path); });
                responder.sendResponse(QHttpServerResponse("text/plain", QByteArray("Queued"), QHttpServerResponse::StatusCode::Ok));
            });

        QTcpServer tcpServer;
        if (!tcpServer.listen(QHostAddress::Any, webhook_port)){
            c_out << "Failed to listen on port " << itoa(webhook_port) << ": " << tcpServer.errorString().toStdString() << std::endl;
            return;
        }

        if (!server.bind(&tcpServer)) {
            c_out << "Failed to bind QHttpServer to QTcpServer" << std::endl;
            return;
        }

        /* Цикл событий сервера завершается, когда запрошена остановка бота */
        QTimer stop_check;
        QObject::connect(&stop_check, &QTimer::timeout, &loop, [&loop] { if (stop_requested()) loop.quit(); });
        stop_check.start(500);
        loop.exec();
    });

    try {
        jira_bot.bot_start(&app);
    }
    catch (std::exception &e){
        c_out << "BotStart's error: " + std::string(e.what()) << std::endl;
    }

    /* bot_start() возвращается после request_stop(), либо по исключению - тогда останавливаем остальное сами */
    request_stop();
    { std::lock_guard lock(stop_mutex); }   /* чтобы служебный поток не пропустил уведомление между проверкой флага и ожиданием */
    stop_cv.notify_all();
    jira_listener.join();
    monitor_mem_size_worker.join();

    send_msg_to_support(jira_bot.m_tg_bot.getApi(), jira_bot.sett["support_TgID"], "⚠️ Бот работу завершил.");
    c_out << "Бот свою работу завершает. Сау булыгыз!" << std::endl;

    return 0;
}
