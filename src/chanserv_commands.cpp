#include "chanserv.h"

void chanserv_bot::on_start_command(Message::Ptr message)
{
    uD = uniq_data(message->from->id);
    if (uD)
    {
        Message::Ptr clicked_message;

        if (is_developer(message->from->id)) {
            clicked_message = m_tg_bot.getApi().sendMessage(message->chat->id, "Режим разработчика", nullptr, nullptr, kb_developer);
        }

        else {
            const auto just_create_keyboard = m_tg_bot.getApi().sendMessage(message->chat->id, "Привет, " + message->from->firstName + "!\n" +
                                                                "Опиши мне свою проблему, а наши специалисты свяжутся с тобой, и решат её.", nullptr, nullptr, kb_ganzel);
            /* Режим v_staff, дополнительные кнопки */
            if (is_staff(message->from->id))
                clicked_message = m_tg_bot.getApi().sendMessage(message->chat->id, "Выбери кнопку",
                                                            nullptr, nullptr, kb_staff);
            else if (is_allowed_contractor(message->from->id))
                clicked_message = m_tg_bot.getApi().sendMessage(message->chat->id, "Выбери кнопку ниже", nullptr, nullptr, kb_contractor);
            else // if (isDBConsumer(message->from->id))
                clicked_message = m_tg_bot.getApi().sendMessage(message->chat->id, "Выберите кнопку ниже", nullptr, nullptr, kb_consumer);
        }
        uD->clicked_msg_id = clicked_message->messageId;
    }
}

void chanserv_bot::handle_commands()
{
    m_tg_bot.getEvents().onCommand("help", [this](Message::Ptr message)
    {
        uD = uniq_data(message->from->id);
        if (uD)
        {
            uD->jira_task = {};              /* Clear the struct */
            c_out << "Пользователю @" << message->from->username << " (" << itoa(message->chat->id) << ") отправлено сообщение 'По вопросам и ошибкам бота можно написать сюда - @tg_devel_tg'" << std::endl;
            m_tg_bot.getApi().sendMessage(message->chat->id, "По вопросам и ошибкам бота можно написать сюда - @tg_devel_tg");
        }
    });

    m_tg_bot.getEvents().onCommand("start", [this](Message::Ptr message)
    {
        uD = uniq_data(message->from->id);
        if (uD)
        {
            /* Обрабатываем запросы - только, если это наш сотрудник */
/*          if (isAllowedConsumer(message->from->id) || isAllowedContractor(message->from->id))                                         */
            {
                uD->jira_task = {};          /* Clear the struct  */
                c_out << message->from->firstName << " (" << itoa(message->from->id) << ") started/restarted the chat" << std::endl;
                on_start_command(message);
            }
        }
    });

}

void chanserv_bot::on_more_photos_command(pgdb_issue const &issue, CallbackQuery::Ptr query)
{
    uD = uniq_data(query->from->id);
    if (uD)
    {
        std::vector<InputMedia::Ptr> media_group;

        for (int i = 0; i < issue.photos.size(); ++i)
        {
            std::shared_ptr<InputMediaPhoto> photo_db_from_blob(new InputMediaPhoto());
            photo_db_from_blob->media = issue.photos[i];
            photo_db_from_blob->hasSpoiler = false;                /* не маскировать "шумами" изображение перед кликом */
            /* photoDBFromBlob->caption = ID_Задачи?;  */
            media_group.push_back(photo_db_from_blob);
        }

        TgBot::ReplyParameters rp_params;
        rp_params.allowSendingWithoutReply = true;
        rp_params.chatId = issue.sent_chat_id;
        rp_params.messageId = issue.sent_msg_id;

        const std::vector<Message::Ptr> sent_album = m_tg_bot.getApi().sendMediaGroup(issue.sent_chat_id, media_group, false, std::make_shared<TgBot::ReplyParameters>(rp_params));
    }
}
