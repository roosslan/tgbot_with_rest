#include "result.h"
#include <optional>
#include <qobject.h>
#include <string>
#include <regex>
#include <vector>

#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>

#pragma once

enum class trello_error { none, unknown, request_failed, request_invalid, not_found_404 };

template <typename T>
using trello_result = result<T, trello_error>;

class jira_card {
public:
    std::string id;
    bool is_closed;
    std::string id_list;
    std::string name;
    std::string description;
    std::optional<std::string> name_simple;
};

/* Класс TrelloCard пока оставлен для совместимости кода, в будущем удалим этот класс */
class trello_card {
    std::string id_;
    std::string id_list_;
    std::string name_;
    std::optional<std::string> name_simple_;
    std::string description_;
    std::optional<std::string> description_simple_;
    std::optional<std::string> id_attachment_cover_;
    std::vector<std::string> labels_;

public:
    trello_card(std::string id, std::string id_list, std::string name, std::string description, std::optional<std::string> id_attachment_cover,
               std::vector<std::string> labels)
        : id_(std::move(id)),
          id_list_(std::move(id_list)),
          name_(std::move(name)),
          description_(std::move(description)),
          id_attachment_cover_(std::move(id_attachment_cover)),
          labels_(std::move(labels)) {}

    const std::string& id() const { return id_; }
    const std::string& id_list() const { return id_list_; }
    const std::string& name() const { return name_; }
    const std::string& description() const { return description_; }
    const std::optional<std::string>& id_attachment_cover() const { return id_attachment_cover_; }
    const std::vector<std::string>& labels() const { return labels_; }
    bool has_label(char const* label) const;

private:
    bool is_match(const std::string& field, const std::string& value);
};

class trello_card_attachment {
    std::string id_;
    std::string name_;
    std::string url_;

public:
    trello_card_attachment(std::string id, std::string name, std::string url)
        : id_(std::move(id)), name_(std::move(name)), url_(std::move(url)) {}

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& url() const { return url_; }
};

typedef trello_result<std::string> trello_string;
typedef trello_result<std::vector<trello_card>> trello_vector;
typedef trello_result<jira_card> jiraCard;
typedef trello_result<trello_card_attachment> trello_attachment;

class trello_api : public QObject
{
    Q_OBJECT

    std::string api_key_;
    std::string user_token_;    
    std::string cache_path_;
    std::string apiprefix_;

    trello_string get_file(const std::string& url, const std::string& extension, bool force = false);
    std::string get_cache_key(const std::string& url);

public:
    trello_api(const std::string api_key, const std::string user_token, const std::string prefix);

    jiraCard get_card(const std::string &card_id);
    static size_t curl_write_callback(char* buff_ptr, size_t sz, size_t cnt_memb, void* userdata);

    int add_img_to_card(const std::string card_id, const std::string imgPath);
    int archiving_card(const std::string& card_id);
    int change_card_color(const std::string& card_id, const std::string& color);
    trello_string create_new_card(const std::string& list_id, std::string card_name, const std::string card_description, std::string** created_card_id);
    trello_string delete_all_cards_from_archive(const std::string list_id);
    trello_vector get_board_cards(const std::string& board_id, bool force = false);

    trello_string get_file(const std::string& url, bool force = false);
    trello_string get_image_file(const std::string& url, std::optional<int> width, std::optional<int> height, bool force = false);    
    trello_attachment get_card_attachment(const std::string& card_id, const std::string& attachment_id);
};
