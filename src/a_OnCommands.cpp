#include "chanserv.h"

void ChanservBot::OnStartCommand(Message::Ptr message)
{
    uD = uniqData(message->from->id);
    if (uD)
    {
        Message::Ptr clickedMessage;

        if (isDeveloper(message->from->id)) {
            clickedMessage = m_tgBot.getApi().sendMessage(message->chat->id, "Режим разработчика", nullptr, nullptr, kbDeveloper);
        }

        else {
            auto justCreateKeyboard = m_tgBot.getApi().sendMessage(message->chat->id, "Привет, " + message->from->firstName + "!\n" +
                                                                "Опиши мне свою проблему, а наши BIM-специалисты свяжутся с тобой, и решат её.", nullptr, nullptr, kbGanzel);
            /* Режим Staff, дополнительные кнопки */
            if (isStaff(message->from->id))
                clickedMessage = m_tgBot.getApi().sendMessage(message->chat->id, "Выбери кнопку",
                                                            nullptr, nullptr, kbStaff);
            else if (isAllowedContractor(message->from->id))
                clickedMessage = m_tgBot.getApi().sendMessage(message->chat->id, "Выбери кнопку ниже", nullptr, nullptr, kbContractor);
            else // if (isDBConsumer(message->from->id))
                clickedMessage = m_tgBot.getApi().sendMessage(message->chat->id, "Выберите кнопку ниже", nullptr, nullptr, kbConsumer);
        }
        uD->clickedMsgId = clickedMessage->messageId;
    }
}

void ChanservBot::HandleCommands()
{
    m_tgBot.getEvents().onCommand("help", [this](Message::Ptr message)
    {
        uD = uniqData(message->from->id);
        if (uD)
        {
            uD->jiraTask = {};              /* Clear the struct */
            c_out << "Пользователю @" << message->from->username << " (" << itoa(message->chat->id) << ") отправлено сообщение 'По вопросам и ошибкам бота можно написать сюда - @alabuga_dev'" << std::endl;
            m_tgBot.getApi().sendMessage(message->chat->id, "По вопросам и ошибкам бота можно написать сюда - @alabuga_dev");
        }
    });

    m_tgBot.getEvents().onCommand("start", [this](Message::Ptr message)
    {
        uD = uniqData(message->from->id);
        if (uD)
        {
            /* Обрабатываем запросы - только, если это наш сотрудник */
/*          if (isAllowedConsumer(message->from->id) || isAllowedContractor(message->from->id))                                         */
            {
                uD->jiraTask = {};          /* Clear the struct  */
                c_out << message->from->firstName << " (" << itoa(message->from->id) << ") started/restarted the chat" << std::endl;
                OnStartCommand(message);
            }
        }
    });

}

void ChanservBot::OnMorePhotosCommand(pgdbIssue const &issue, CallbackQuery::Ptr query)
{
    uD = uniqData(query->from->id);
    if (uD)
    {
        std::vector<InputMedia::Ptr> mediaGroup;

        for (int i = 0; i < issue.Photos.size(); ++i)
        {
            std::shared_ptr<InputMediaPhoto> photoDBFromBlob(new InputMediaPhoto());
            photoDBFromBlob->media = issue.Photos[i];
            photoDBFromBlob->hasSpoiler = false;                /* не маскировать "шумами" изображение перед кликом */
            /* photoDBFromBlob->caption = ID_Задачи?;  */
            mediaGroup.push_back(photoDBFromBlob);
        }

        TgBot::ReplyParameters rp_params;
        rp_params.allowSendingWithoutReply = true;
        rp_params.chatId = issue.sentChatID;
        rp_params.messageId = issue.sentMsgID;

        std::vector<Message::Ptr> sentAlbum = m_tgBot.getApi().sendMediaGroup(issue.sentChatID, mediaGroup, false, std::make_shared<TgBot::ReplyParameters>(rp_params));
    }
}
