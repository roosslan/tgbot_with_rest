#include "maxbotapi.h"

Task<> welcome_task(Client client, std::shared_ptr<manapi::async::context>& Context) {
    auto& [user_Id, _, bot] = client;
//    auto& [chatId, _, __, ___, bot] = client;
    const std::string welcomeMessage =
        "Это тестовый текст!\n По Текле \n В любой момент введите /start для перезапуска";
//    manapi::async::context::current(Context);
//    manapi::async::run ( [client, chatId] () mutable MANAPIHTTP_NOEXCEPT -> manapi::future<> {
        auto res_send = co_await client.m_Bot.send_message(client.user_Id, { { "text", std::format("B Jira появилось новое обращение") } });
//        co_return; });
//    auto res_send = co_await bot.send_message(user_Id, "welcomeMessage");
//    BOOST_LOG_TRIVIAL(debug) << "Sent welcome message to chat " << chatId;
    co_return;
}

Task<> client_entry(int64_t user_Id, MessageQueue<std::string>& msgQueue, manapi::maxbot& bot, std::shared_ptr<manapi::async::context>& Context) {
    // Connects to the server as a new client
    // Create a new client to pass to children tasks/functions

    Client client = {user_Id, msgQueue, bot};

    co_await msgQueue;                  // remove the starting message if any
    co_await welcome_task(client, Context);       // Display welcome message to user

    // === Game has started
    // All execution traces that lead to here have the socket subscribing to the info messages
    while (true) {
        auto res_send = co_await bot.send_message(user_Id, { { "text", std::format( "Тестовая проверка async co_await") } });

        const std::string text = co_await msgQueue;
        res_send = co_await bot.send_message(user_Id, { { "text", " action is: " + text } });
        // process using replyBytes
    }

    co_return;
}

Task<> welcome_task(tgClient client, const TgBot::Api& tgbot) {
    auto& [chatId, _, bot] = client;
    const std::string welcomeMessage =
        "Это тестовый текст!\n Для Теклы \n В любой момент введите /start для перезапуска";
    client.m_tgBot.sendMessage(client.user_Id, welcomeMessage);
    co_return;
}

Task<> client_entry(int64_t user_Id, MessageQueue<std::string>& msg_queue, const TgBot::Api& tg_bot) {
    // Connects to the server as a new client
    // Create a new client to pass to children tasks/functions

    tgClient client = {user_Id, msg_queue, tg_bot};

    co_await msg_queue;                      // remove the starting message if any
    co_await welcome_task(client, tg_bot);       // Display welcome message to user

    // === Dialog has started
    // All execution traces that lead to here have the socket subscribing to the info messages
    while (true) {
        tg_bot.sendMessage(user_Id, "Тестовая проверка без async co_await");

        const std::string text = co_await msg_queue;
        tg_bot.sendMessage(user_Id, "Your action is: " + text);
    }

    co_return;
}
