#include "chanserv.h"

void chanserv_bot::create_one_column_keyboard(const std::vector<std::string>& button_strings, ReplyKeyboardMarkup::Ptr& kb)
{
    for (size_t i = 0; i < button_strings.size(); ++i)
    {
        std::vector<KeyboardButton::Ptr> row;
        KeyboardButton::Ptr button(new KeyboardButton);
        button->requestContact = false;
        button->requestLocation = false;
        button->text = button_strings[i];
        row.push_back(button);
        kb->keyboard.push_back(row);
    }
}

void chanserv_bot::create_keyboard(const std::vector<std::vector<std::string>>& button_layout, ReplyKeyboardMarkup::Ptr& kb)
{
    for (size_t i = 0; i < button_layout.size(); ++i)
    {
        std::vector<KeyboardButton::Ptr> row;
        for (size_t j = 0; j < button_layout[i].size(); ++j)
        {
            KeyboardButton::Ptr button(new KeyboardButton);
            button->requestContact = false;
            button->requestLocation = false;
            button->text = button_layout[i][j];
            row.push_back(button);
        }
        kb->keyboard.push_back(row);
    }
}

int chanserv_bot::create_keyboards()
{
    kb_ganzel = std::shared_ptr<ReplyKeyboardMarkup>( new ReplyKeyboardMarkup );
    kb_ganzel->isPersistent = true;
    kb_ganzel->resizeKeyboard = true;
    kb_ganzel->oneTimeKeyboard = true;
    create_one_column_keyboard({"/start"}, kb_ganzel);

    /* NEW INLINE /START MENU ================================================================ */

    std::vector<InlineKeyboardButton::Ptr> row_new_task_command;
    kb_consumer = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    btn_new_task = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_new_task->text = "Новая задача";
    btn_new_task->callbackData = "new_task";
    row_new_task_command.push_back(btn_new_task);

    std::vector<InlineKeyboardButton::Ptr> row_new_family_command;
    btn_new_family = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_new_family->text = "Новое семейство";
    btn_new_family->callbackData = "new_family";
    row_new_family_command.push_back(btn_new_family);
/*
    std::vector<InlineKeyboardButton::Ptr> row_new_tekla_command;
    btn_new_tekla = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_new_tekla->text = "Новое обращение Tekla";
    btn_new_tekla->callbackData = "tekla_enter_menu";
    row_new_tekla_command.push_back(btn_new_tekla);

    /* KB CONSUMER =========================================================================== */

    QSqlQuery pg_query(pgDatabase);
    pg_query.exec("SELECT * FROM buttons WHERE role = 'consumer'");
    int pg_buttons_count = pg_query.numRowsAffected();

    if (pg_buttons_count > 0)
        for (int i = 0; i < pg_buttons_count; ++i)
        {
            InlineKeyboardButton::Ptr pg_button = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
            pg_query.next();
            pg_button->text = pg_query.value(1).toString().toStdString();
            pg_button->callbackData = pg_query.value(2).toString().toStdString();

            consumer_buttons.push_back(pg_button);
        }
    kb_consumer = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    if (!consumer_buttons.empty())
        kb_consumer->inlineKeyboard.push_back(consumer_buttons);

    kb_consumer->inlineKeyboard.push_back(row_new_task_command);
    kb_consumer->inlineKeyboard.push_back(row_new_family_command);

 /* kb_consumer->inlineKeyboard.push_back(row_new_tekla_command);   */

    /* KB CONTRACTOR ========================================================================= */
/*
    kb_contractor = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_my_tasks;
    btn_my_tasks = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_my_tasks->text = "Мои задачи";
    btn_my_tasks->callbackData = "my_tasks";
    row_my_tasks.push_back(btn_my_tasks);

    kb_contractor->inlineKeyboard.push_back(row_my_tasks);
*/
    pg_query.clear();
    pg_buttons_count = 0;
    pg_query.exec("SELECT * FROM buttons WHERE role = 'contractor'");
    pg_buttons_count = pg_query.numRowsAffected();

    if (pg_buttons_count > 0)
        for (int i = 0; i < pg_buttons_count; ++i)
        {
            InlineKeyboardButton::Ptr pg_button = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
            pg_query.next();
            pg_button->text = pg_query.value(1).toString().toStdString();
            pg_button->callbackData = pg_query.value(2).toString().toStdString();

            contractor_buttons.push_back(pg_button);
        }
    kb_contractor = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    if (!contractor_buttons.empty())
        kb_contractor->inlineKeyboard.push_back(contractor_buttons);

    kb_contractor->inlineKeyboard.push_back(row_new_task_command);
    kb_contractor->inlineKeyboard.push_back(row_new_family_command);

/*  kb_contractor->inlineKeyboard.push_back(row_new_tekla_command);     */

    /* KB STAFF ============================================================================== */

    pg_query.clear();
    pg_buttons_count = 0;
    pg_query.exec("SELECT * FROM buttons WHERE role = 'staff'");
    pg_buttons_count = pg_query.numRowsAffected();

    if (pg_buttons_count > 0)
        for (int i = 0; i < pg_buttons_count; ++i)
        {
            InlineKeyboardButton::Ptr pg_button = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
            pg_query.next();
            pg_button->text = pg_query.value(1).toString().toStdString();
            pg_button->callbackData = pg_query.value(2).toString().toStdString();

            staff_buttons.push_back(pg_button);
        }
    kb_staff = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    if (!staff_buttons.empty())
        kb_staff->inlineKeyboard.push_back(staff_buttons);

/*
    kb_staff = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_staff;
    btn_edit_msi_version = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_edit_msi_version->text = "Обновить версию MSI сборки";
    btn_edit_msi_version->callbackData = "set_new_msi_version";
    row_staff.push_back(btn_edit_msi_version);

    btn_add_new_contractor = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_add_new_contractor->text = "Добавить нового исполнителя";
    btn_add_new_contractor->callbackData = "add_new_contractor";
    row_staff.push_back(btn_add_new_contractor);

    kb_staff->inlineKeyboard.push_back(row_staff);
/*  kb_staff->inlineKeyboard.push_back(row_my_tasks);           */

    kb_staff->inlineKeyboard.push_back(row_new_task_command);
    kb_staff->inlineKeyboard.push_back(row_new_family_command);    

    /* DEVELOPER MENU ======================================================================== */

    pg_query.clear();
    pg_buttons_count = 0;
    pg_query.exec("SELECT * FROM buttons WHERE role = 'developer'");
    pg_buttons_count = pg_query.numRowsAffected();

    if (pg_buttons_count > 0)
    for (int i = 0; i < pg_buttons_count; ++i)
    {
        InlineKeyboardButton::Ptr pg_button = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
        pg_query.next();
        pg_button->text = pg_query.value(1).toString().toStdString();
        pg_button->callbackData = pg_query.value(2).toString().toStdString();

        dev_buttons.push_back(pg_button);
    }
    kb_developer = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    if (!dev_buttons.empty())
        kb_developer->inlineKeyboard.push_back(dev_buttons);

/*
    std::vector<InlineKeyboardButton::Ptr> row_developer;

    btn_send_file = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_send_file->text = "Разослать всем файл";
    btn_send_file->callbackData = "send_file";

    btn_run_shell_command = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_run_shell_command->text = "Выполнить команду на хосте";
    btn_run_shell_command->callbackData = "run_command";

    btn_msg_as_bot = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_msg_as_bot->text = "Сообщение от имени бота";
    btn_msg_as_bot->callbackData = "msg_as_bot";

    btn_forward_to_gpt = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_forward_to_gpt->text = "Перейти в режим GTP 4-o";
    btn_forward_to_gpt->callbackData = "msg_to_gpt";

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
    row_developer2.push_back(btnAddNewContractor);

    row_developer.push_back(btn_run_shell_command);
    row_developer.push_back(btn_msg_as_bot);

    /* row_new_tekla_command.push_back(btnSendFile);
    row_new_tekla_command.push_back(btn_new_task);

    kb_developer->inlineKeyboard.push_back(row_new_tekla_command);

    kb_developer->inlineKeyboard.push_back(row_developer);  */


/*  keyboardGuest = std::shared_ptr<ReplyKeyboardMarkup>( new ReplyKeyboardMarkup );
    keyboardGuest->resizeKeyboard = true;
    createKeyboard({{"Добавить меня в постановщики задач!"}}, keyboardGuest);
*/

    /* NEW_TEKLA ============================================================================ */

    std::vector<InlineKeyboardButton::Ptr> row_improve_suggestions;
    btn_improve_suggest = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_improve_suggest->text = "Предложение по улучшению работы";
    btn_improve_suggest->callbackData = "tekla_improve_suggestion";
    row_improve_suggestions.push_back(btn_improve_suggest);

    std::vector<InlineKeyboardButton::Ptr> row_model_repair;
    kb_tekla = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    btn_model_repair = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_model_repair->text = "Поломка модели, прошу восстановить";
    btn_model_repair->callbackData = "tekla_model_repair";
    row_model_repair.push_back(btn_model_repair);

    std::vector<InlineKeyboardButton::Ptr> row_directory_access;
    btn_directory_access = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_directory_access->text = "Нужен доступ к папке";
    btn_directory_access->callbackData = "tekla_directory_access";
    row_directory_access.push_back(btn_directory_access);

    std::vector<InlineKeyboardButton::Ptr> row_general_issue;
    btn_general_issue = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_general_issue->text = "ЦУП, у  нас проблема";
    btn_general_issue->callbackData = "tekla_general_issue";
    row_general_issue.push_back(btn_general_issue);

    std::vector<InlineKeyboardButton::Ptr> row_tekla_back;
    btn_tekla_back = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_tekla_back->text = "Назад";
    btn_tekla_back->callbackData = "tekla_back";
    row_tekla_back.push_back(btn_tekla_back);

/*  kbTekla->inlineKeyboard.push_back(row_improve_suggestions);
    kbTekla->inlineKeyboard.push_back(row_model_repair);
    kbTekla->inlineKeyboard.push_back(row_directory_access);
*/
    kb_tekla->inlineKeyboard.push_back(row_general_issue);
    kb_tekla->inlineKeyboard.push_back(row_tekla_back);

/* ПРИСТУПИТЬ К ВЫПОЛНЕНИЮ! ================================================================= */

    kb_start_solving = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    btn_start_solving = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_start_solving->text = "Приступить к выполнению 🫡";                 /* press F" emoji */
    std::vector<InlineKeyboardButton::Ptr> row_start_solving_options;
    btn_start_solving->callbackData = "start_solving";
    row_start_solving_options.push_back(btn_start_solving);
    kb_start_solving->inlineKeyboard.push_back(row_start_solving_options);

    kb_start_solving_w_photo = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    btn_more_photos = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_more_photos->text = "Больше фото 🖼️";                 /* Picture emoji */
    btn_more_photos->callbackData = "more_photos";
    row_start_solving_options.push_back(btn_more_photos);
    kb_start_solving_w_photo->inlineKeyboard.push_back(row_start_solving_options);

/* ЗАВЕРШИТЬ ВЫПОЛНЕНИЕ! ===================================================================== */

    kb_task_closing = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_task_closing_options;
    btn_task_closing = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_task_closing->text = "Завершить выполнение ⚡️";
    btn_task_closing->callbackData = "task_closing";
    row_task_closing_options.push_back(btn_task_closing);
    kb_task_closing->inlineKeyboard.push_back(row_task_closing_options);


 /* ЗАДАЧА ВЫПОЛНЕНА! ======================================================================== */

    kb_task_done = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_task_done_options;
    btn_task_done = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_task_done->text = "Задача выполнена ✅";                   /* Checked checkbox "V" emoji */
    btn_task_done->callbackData = "task_done";
    row_task_done_options.push_back(btn_task_done);
    kb_task_done->inlineKeyboard.push_back(row_task_done_options);

    kb_go_dashboard = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    std::vector<InlineKeyboardButton::Ptr> row_go_dashboard_options;
    btn_go_dashboard = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_go_dashboard->text = "К доске! 🚀";                       /* Rocket emoji */
    btn_go_dashboard->url = sett["board_link"];
    row_go_dashboard_options.push_back(btn_go_dashboard);
    kb_go_dashboard->inlineKeyboard.push_back(row_go_dashboard_options);

/* НЕТ БОЛЬШЕ ФОТО ======================================================================== */

    kb_manage_photos = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    btn_no_more_photos = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    btn_no_more_photos->text = "Нет больше фото 📷";
    btn_no_more_photos->callbackData = "no_more_photos";
    std::vector<InlineKeyboardButton::Ptr> row_no_more_photos;
    row_no_more_photos.push_back(btn_no_more_photos);
    kb_manage_photos->inlineKeyboard.push_back(row_no_more_photos);

/*  inlinePhotoKb = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
    inlinePhotoKb->inlineKeyboard.push_back(row_photo_options);

    attachPhotoButton = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
    attachPhotoButton->text = "Прикрепить 📎";
    attachPhotoButton->callbackData = "attach_photo";
    row_photo_options.push_back(attachPhotoButton);     */

    return 0;
}

