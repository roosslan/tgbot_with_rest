#ifndef MAXBOTAPI_H
#define MAXBOTAPI_H

#include <iostream>
#include <queue>
#include <string>
#include <cassert>
#include <coroutine>

#include <tgbot/tgbot.h>
#include <tgbot/Bot.h>
#include <manapihttp/ManapiInitTools.hpp>
#include "tm_bot.hpp"

// Lazily started coroutine returning a Result
template <typename Result = void>
class [[nodiscard]] Task {
public:
    struct FinalAwaiter {  // awaiter that is called when the final suspend point is reached
        bool await_ready() const noexcept { return false; }
        template <typename P>
        void await_suspend(std::coroutine_handle<P> handle) noexcept {
            if (handle.promise().continuation) {
                handle.promise().continuation.resume();  // resume the caller's coroutine
            }
            // don't resume if not finishing on a co_await
        }
        void await_resume() const noexcept {}
    };

    // Minimal coroutine return type
    struct Promise {
        std::coroutine_handle<> continuation{nullptr};
        Result result;
        Task get_return_object() {
            return Task{std::coroutine_handle<Promise>::from_promise(*this)};
        }

        void unhandled_exception() noexcept {}
        void return_value(Result&& res) noexcept { result = std::move(res); }
        std::suspend_always initial_suspend() noexcept { return {}; }  // suspend immediately, the caller should put the coroutine on the stack to allow message passing
        FinalAwaiter final_suspend() noexcept { return {}; }
    };
    using promise_type = Promise;
    std::coroutine_handle<Promise> handle_{nullptr};

    struct Awaiter {  // used when we call co_await on a task
        // The way we use this, since our task doesn't do co_yield, this should only run once
        std::coroutine_handle<Promise> handle;
        bool await_ready() const noexcept { return false; }
        auto await_suspend(std::coroutine_handle<> calling) noexcept {
            handle.promise().continuation = calling;  // store the caller's coroutine handle in the promise
            return handle;                            // symmetric transfer to start the calee coroutinew
        }

        template <typename T = Result>
            requires std::is_same_v<T, void>
        void await_resume() noexcept {}

        template <typename T = Result>
            requires(!std::is_same_v<T, void>)
        T await_resume() noexcept {
            return std::move(handle.promise().result);
        }
    };

    // Task is awaitable since it has a co_await operator that returns an awaiter
    auto operator co_await() noexcept {
        if (!handle_) {
/*          BOOST_LOG_TRIVIAL(fatal) << "Task is not properly initialised!";    */
        }
        return Awaiter{handle_};
    }

    // Tasks are not copyable, only movable
    Task(const Task&) = delete;
    Task&(operator=(const Task&)) = delete;
    Task(Task&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    ~Task() {  // destroy the coroutine when the task is destroyed
        if (handle_) {
/*          BOOST_LOG_TRIVIAL(trace) << "Destroying coroutine";     */
            handle_.destroy();
        }
    }

private:
    explicit Task(std::coroutine_handle<Promise> handle)
        : handle_(handle) {
    }
};

template <>
struct Task<void>::Promise {
    std::coroutine_handle<> continuation{nullptr};

    Task get_return_object() {
        return Task{std::coroutine_handle<Promise>::from_promise(*this)};
    }

    void unhandled_exception() noexcept {}
    void return_void() noexcept {}

    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalAwaiter final_suspend() noexcept { return {}; }
};

template <typename T>
class MessageQueue {
public:
    class AwaiterForMessage {
    public:
        AwaiterForMessage(MessageQueue<T>& msgQueue) : msgQueue_{msgQueue} {}

        bool await_ready() const noexcept {
            return !msgQueue_.queue_.empty();
        }

        void await_suspend(std::coroutine_handle<> handle) {
            msgQueue_.m_awaiter = handle;
        }

        T await_resume() {
            T value = std::move(msgQueue_.queue_.front());
            msgQueue_.queue_.pop();
            return value;
        }

    private:
        MessageQueue<T>& msgQueue_;
    };

    auto operator co_await() noexcept {
        return AwaiterForMessage(*this);
    }

    MessageQueue() = default;
    std::queue<T> queue_{};                      // the queue of messages
    std::coroutine_handle<> m_awaiter{nullptr};  // the coroutine handle of the current awaiter

    void pushMessage(T message) {
        queue_.push(message);
        if (m_awaiter && !m_awaiter.done()) {  // if there is a coroutine waiting for a message
            m_awaiter.resume();
        }
    }
};

struct Client { // struct of references of the client context to pass around
    int64_t& user_Id;
    MessageQueue<std::string>& msgQueue;
    manapi::maxbot& m_Bot;
};

struct tgClient {
    int64_t& user_Id;
    MessageQueue<std::string>& msgQueue;
    const TgBot::Api& m_tgBot;
};

Task<> welcome_task(Client client, std::shared_ptr<manapi::async::context>& Context);
Task<> welcome_task(Client client, TgBot::Bot& tg_bot);

Task<> client_entry(int64_t user_Id, MessageQueue<std::string>& msgQueue, manapi::maxbot& bot, std::shared_ptr<manapi::async::context>& Context);
Task<> client_entry(int64_t user_Id, MessageQueue<std::string>& msg_queue, const TgBot::Api& tg_bot);

class ChatSessions {
private:
    manapi::maxbot bot_;                  /* bot to send messages    */
    const TgBot::Api tgbot_;              /* tgbot to send messages  */
    std::shared_ptr<manapi::async::context> context;
    std::unordered_map<int64_t, Task<void>> chatCoroutines;
    std::unordered_map<int64_t, MessageQueue<std::string>> chatMessageQueues;

public:
    ChatSessions(manapi::maxbot& bot_, const TgBot::Api &tgbot_, std::shared_ptr<manapi::async::context> &context_) : bot_(bot_), tgbot_(tgbot_), context(context_) {}
    ChatSessions(const TgBot::Api &tgbot_) : tgbot_(tgbot_) {}

    bool hasSession(const int64_t user_Id) const {
        if (!chatCoroutines.contains(user_Id)) return false;
        if (!chatCoroutines.at(user_Id).handle_) return false;
        if (chatCoroutines.at(user_Id).handle_.done()) return false;
        return true;
    }

    void createNewTgSession(const int64_t user_Id) {
        // new clients are handled via clientEntry coroutine
        chatMessageQueues.insert_or_assign(user_Id, MessageQueue<std::string>());
        chatCoroutines.insert_or_assign(user_Id,
                                        client_entry(user_Id, chatMessageQueues.at(user_Id), tgbot_));
        chatCoroutines.at(user_Id).handle_.resume();  // start the coroutine
    }

    void createNewSession(const int64_t user_Id) {
        // new clients are handled via clientEntry coroutine
        chatMessageQueues.insert_or_assign(user_Id, MessageQueue<std::string>());
        chatCoroutines.insert_or_assign(user_Id,
                                        client_entry(user_Id, chatMessageQueues.at(user_Id), bot_, context));
        chatCoroutines.at(user_Id).handle_.resume();  // start the coroutine
    }

    void passMessage(const int64_t user_Id, std::string message) {
        chatMessageQueues.at(user_Id).pushMessage(std::move(message));
    }

    void deleteSession(const int64_t user_Id) {
        chatCoroutines.erase(user_Id);
        chatMessageQueues.erase(user_Id);
    }
};


#endif // MAXBOTAPI_H
