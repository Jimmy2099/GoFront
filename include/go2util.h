#ifndef GO2_UTIL_H
#define GO2_UTIL_H

#include <iostream>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <utility>

namespace go2 {

class deferred_print {
    std::string value;

public:
    explicit deferred_print(std::string value_) : value{std::move(value_)} { }
    deferred_print(deferred_print const&) = delete;
    deferred_print(deferred_print&&) = default;
    ~deferred_print() { std::cout << value << '\n'; }
};

template<typename T>
class channel {
    struct state {
        std::mutex mutex;
        std::condition_variable can_read;
        std::condition_variable can_write;
        std::queue<T> values;
        std::size_t capacity;
        explicit state(std::size_t capacity_) : capacity{capacity_} { }
    };
    std::shared_ptr<state> data;

public:
    explicit channel(std::size_t capacity_ = 1) : data{std::make_shared<state>(capacity_)} { }
    void send(T value) {
        std::unique_lock lock{data->mutex};
        data->can_write.wait(lock, [&] { return data->values.size() < data->capacity; });
        data->values.push(std::move(value));
        data->can_read.notify_one();
    }
    auto receive() -> T {
        std::unique_lock lock{data->mutex};
        data->can_read.wait(lock, [&] { return !data->values.empty(); });
        auto value = std::move(data->values.front());
        data->values.pop();
        data->can_write.notify_one();
        return value;
    }
};

template<typename F>
auto go(F&& function) -> void {
    std::thread{std::forward<F>(function)}.detach();
}

template<typename F, typename... Args>
auto go_call(F&& function, Args&&... args) -> void {
    std::thread{
        [
            function = std::forward<F>(function),
            arguments = std::make_tuple(std::forward<Args>(args)...)
        ]() mutable {
            std::apply(
                [&function](auto&... values) {
                    function(values...);
                },
                arguments
            );
        }
    }.detach();
}

}

#endif
