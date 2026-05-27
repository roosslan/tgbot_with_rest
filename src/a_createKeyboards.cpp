#include "chanserv.h"

void ChanservBot::CreateOneColumnKeyboard(const std::vector<std::string>& buttonStrings, ReplyKeyboardMarkup::Ptr& kb)
{
    for (size_t i = 0; i < buttonStrings.size(); ++i)
    {
        std::vector<KeyboardButton::Ptr> row;
        KeyboardButton::Ptr button(new KeyboardButton);
        button->requestContact = false;
        button->requestLocation = false;
        button->text = buttonStrings[i];
        row.push_back(button);
        kb->keyboard.push_back(row);
    }
}

void ChanservBot::CreateKeyboard(const std::vector<std::vector<std::string>>& buttonLayout, ReplyKeyboardMarkup::Ptr& kb)
{
    for (size_t i = 0; i < buttonLayout.size(); ++i)
    {
        std::vector<KeyboardButton::Ptr> row;
        for (size_t j = 0; j < buttonLayout[i].size(); ++j)
        {
            KeyboardButton::Ptr button(new KeyboardButton);
            button->requestContact = false;
            button->requestLocation = false;
            button->text = buttonLayout[i][j];
            row.push_back(button);
        }
        kb->keyboard.push_back(row);
    }
}

int ChanservBot::CreateKeyboards()
{
    kbGanzel = std::shared_ptr<ReplyKeyboardMarkup>( new ReplyKeyboardMarkup );
    kbGanzel->isPersistent = true;
    kbGanzel->resizeKeyboard = true;
    kbGanzel->oneTimeKeyboard = true;
    CreateOneColumnKeyboard({"/start"}, kbGanzel);

    /* NEW INLINE /START MENU ================================================================ */

    std::vector<InlineKeyboardButton::Ptr> row_new_task_command;
    kbConsumer = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    btnNewTask = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnNewTask->text = "Новая задача";
    btnNewTask->callbackData = "new_task";
    row_new_task_command.push_back(btnNewTask);

    std::vector<InlineKeyboardButton::Ptr> row_new_family_command;
    btnNewFamily = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnNewFamily->text = "Новое семейство";
    btnNewFamily->callbackData = "new_family";
    row_new_family_command.push_back(btnNewFamily);

    std::vector<InlineKeyboardButton::Ptr> row_new_tekla_command;
    btnNewTekla = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnNewTekla->text = "Новое обращение Tekla";
    btnNewTekla->callbackData = "tekla_enter_menu";
    row_new_tekla_command.push_back(btnNewTekla);

    kbConsumer->inlineKeyboard.push_back(row_new_task_command);
    kbConsumer->inlineKeyboard.push_back(row_new_family_command);
    kbConsumer->inlineKeyboard.push_back(row_new_tekla_command);

    kbContractor = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_my_tasks;
    btnMyTasks = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnMyTasks->text = "Мои задачи";
    btnMyTasks->callbackData = "my_tasks";    
    row_my_tasks.push_back(btnMyTasks);

    /*
    std::vector<InlineKeyboardButton::Ptr> row_master_key;
    btnReqMasterKey = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnReqMasterKey->text = "Запрос мастер-ключа для Revit";
    btnReqMasterKey->callbackData = "req_master_key";
    row_master_key.push_back(btnReqMasterKey);
    */

    kbContractor->inlineKeyboard.push_back(row_my_tasks);
    kbContractor->inlineKeyboard.push_back(row_new_task_command);
    kbContractor->inlineKeyboard.push_back(row_new_family_command);
/*  kbContractor->inlineKeyboard.push_back(row_master_key);         */
    kbContractor->inlineKeyboard.push_back(row_new_tekla_command);

    /* KB STAFF ============================================================================== */

    kbStaff = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_staff;
    btnEditMSIVersion = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnEditMSIVersion->text = "Обновить версию MSI сборки";
    btnEditMSIVersion->callbackData = "set_new_msi_version";
    row_staff.push_back(btnEditMSIVersion);

    btnAddNewContractor = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnAddNewContractor->text = "Добавить нового исполнителя";
    btnAddNewContractor->callbackData = "add_new_contractor";
    row_staff.push_back(btnAddNewContractor);

    kbStaff->inlineKeyboard.push_back(row_my_tasks);
    kbStaff->inlineKeyboard.push_back(row_new_task_command);
    kbStaff->inlineKeyboard.push_back(row_new_family_command);
/*  kbStaff->inlineKeyboard.push_back(row_master_key);          */
    kbStaff->inlineKeyboard.push_back(row_staff);

    /* DEVELOPER MENU ======================================================================== */

    QSqlQuery pgQuery(pgDatabase);
    pgQuery.exec("SELECT * FROM buttons WHERE role = 'developer'");
    int pgButtonsCount = pgQuery.numRowsAffected();

    if (pgButtonsCount > 0)
    for (int i = 0; i < pgButtonsCount; ++i)
    {
        InlineKeyboardButton::Ptr pgButton = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
        pgQuery.next();
        pgButton->text = pgQuery.value(1).toString().toStdString();
        pgButton->callbackData = pgQuery.value(2).toString().toStdString();

        devButtons.push_back(pgButton);
    }
    kbDeveloper = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    if (!devButtons.empty())
        kbDeveloper->inlineKeyboard.push_back(devButtons);

    std::vector<InlineKeyboardButton::Ptr> row_developer;



    btnSendFile = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnSendFile->text = "Разослать всем файл";
    btnSendFile->callbackData = "send_file";

    btnRunShellCommand = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnRunShellCommand->text = "Выполнить команду на хосте";
    btnRunShellCommand->callbackData = "run_command";

    btnMsgAsBot = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnMsgAsBot->text = "Сообщение от имени бота";
    btnMsgAsBot->callbackData = "msg_as_bot";

    btnForwardToGPT = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnForwardToGPT->text = "Перейти в режим GTP 4-o";
    btnForwardToGPT->callbackData = "msg_to_gpt";
/*
    btnRebootHost = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnRebootHost->text = "Перезагрузить хост";
    btnRebootHost->callbackData = "reboot_host";
    row_developer2.push_back(btnRebootHost);
    row_developer2.push_back(btnReqMasterKey);
    row_developer2.push_back(btnForwardToGPT);

/*  kbDeveloper->inlineKeyboard.push_back(row_my_tasks);
    kbDeveloper->inlineKeyboard.push_back(row_new_task_command);
    kbDeveloper->inlineKeyboard.push_back(row_new_family_command);
    row_new_tekla_command.push_back(btnEditMSIVersion);
    row_developer2.push_back(btnAddNewContractor);      */


    row_developer.push_back(btnRunShellCommand);
    row_developer.push_back(btnMsgAsBot);

    /* row_new_tekla_command.push_back(btnSendFile);   */
    row_new_tekla_command.push_back(btnNewTask);

    kbDeveloper->inlineKeyboard.push_back(row_new_tekla_command);
    kbDeveloper->inlineKeyboard.push_back(row_developer);


/*  keyboardGuest = std::shared_ptr<ReplyKeyboardMarkup>( new ReplyKeyboardMarkup );
    keyboardGuest->resizeKeyboard = true;
    createKeyboard({{"Добавить меня в постановщики задач!"}}, keyboardGuest);
*/

    /* NEW_TEKLA ============================================================================ */

    std::vector<InlineKeyboardButton::Ptr> row_improve_suggestions;
    btnImproveSuggest = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnImproveSuggest->text = "Предложение по улучшению работы";
    btnImproveSuggest->callbackData = "tekla_improve_suggestion";
    row_improve_suggestions.push_back(btnImproveSuggest);

    std::vector<InlineKeyboardButton::Ptr> row_model_repair;
    kbTekla = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    btnModelRepair = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnModelRepair->text = "Поломка модели, прошу восстановить";
    btnModelRepair->callbackData = "tekla_model_repair";
    row_model_repair.push_back(btnModelRepair);

    std::vector<InlineKeyboardButton::Ptr> row_directory_acceess;
    btnDirectoryAccesss = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnDirectoryAccesss->text = "Нужен доступ к папке";
    btnDirectoryAccesss->callbackData = "tekla_directory_access";
    row_directory_acceess.push_back(btnDirectoryAccesss);

    std::vector<InlineKeyboardButton::Ptr> row_general_issue;
    btnGeneralIssue = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnGeneralIssue->text = "ЦУП, у  нас проблема";
    btnGeneralIssue->callbackData = "tekla_general_issue";
    row_general_issue.push_back(btnGeneralIssue);

    std::vector<InlineKeyboardButton::Ptr> row_tekla_back;
    btnTeklaBack = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnTeklaBack->text = "Назад";
    btnTeklaBack->callbackData = "tekla_back";
    row_tekla_back.push_back(btnTeklaBack);

/*  kbTekla->inlineKeyboard.push_back(row_improve_suggestions);
    kbTekla->inlineKeyboard.push_back(row_model_repair);
    kbTekla->inlineKeyboard.push_back(row_directory_acceess);
*/
    kbTekla->inlineKeyboard.push_back(row_general_issue);
    kbTekla->inlineKeyboard.push_back(row_tekla_back);

/* ПРИСТУПИТЬ К ВЫПОЛНЕНИЮ! ================================================================= */

    kbStartSolving = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    btnStartSolving = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnStartSolving->text = "Приступить к выполнению 🫡";               /* press F" emoji */
    std::vector<InlineKeyboardButton::Ptr> row_start_solving_options;
    btnStartSolving->callbackData = "start_solving";
    row_start_solving_options.push_back(btnStartSolving);
    kbStartSolving->inlineKeyboard.push_back(row_start_solving_options);

    kbStartSolvingWPhoto = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    btnMorePhotos = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnMorePhotos->text = "Больше фото 🖼️";                 /* Picture emoji */
    btnMorePhotos->callbackData = "more_photos";
    row_start_solving_options.push_back(btnMorePhotos);
    kbStartSolvingWPhoto->inlineKeyboard.push_back(row_start_solving_options);

/* ЗАВЕРШИТЬ ВЫПОЛНЕНИЕ! ===================================================================== */

    kbTaskClosing = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_task_closing_options;
    btnTaskClosing = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnTaskClosing->text = "Завершить выполнение ⚡️";
    btnTaskClosing->callbackData = "task_closing";
    row_task_closing_options.push_back(btnTaskClosing);
    kbTaskClosing->inlineKeyboard.push_back(row_task_closing_options);


 /* ЗАДАЧА ВЫПОЛНЕНА! ======================================================================== */

    kbTaskDone = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_task_done_options;
    btnTaskDone = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnTaskDone->text = "Задача выполнена ✅";                   /* Checked checkbox "V" emoji */
    btnTaskDone->callbackData = "task_done";
    row_task_done_options.push_back(btnTaskDone);
    kbTaskDone->inlineKeyboard.push_back(row_task_done_options);

    kbGoDashboard = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_go_dashboard_options;
    btnGoDashboard = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnGoDashboard->text = "К доске! 🚀";                       /* Rocket emoji */
    btnGoDashboard->url = sett["board_link"];
    row_go_dashboard_options.push_back(btnGoDashboard);
    kbGoDashboard->inlineKeyboard.push_back(row_go_dashboard_options);

/* НЕТ БОЛЬШЕ ФОТО ======================================================================== */

    kbManagePhotos = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    btnNoMorePhotos = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btnNoMorePhotos->text = "Нет больше фото 📷";
    btnNoMorePhotos->callbackData = "no_more_photos";
    std::vector<InlineKeyboardButton::Ptr> row_no_more_photos;
    row_no_more_photos.push_back(btnNoMorePhotos);
    kbManagePhotos->inlineKeyboard.push_back(row_no_more_photos);

/*  inlinePhotoKb = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    inlinePhotoKb->inlineKeyboard.push_back(row_photo_options);

    attachPhotoButton = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    attachPhotoButton->text = "Прикрепить 📎";
    attachPhotoButton->callbackData = "attach_photo";
    row_photo_options.push_back(attachPhotoButton);     */

    return 0;
}

