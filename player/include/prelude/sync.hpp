#pragma once

#include <queue>
#include <mutex>
#include <optional>

namespace sync {

template <class T>
class Queue {
public:
    using Item = T;

    void push(Item && item) {
        lock_t lock { _mutex };

        _queue.push(std::move(item));
    }

    std::optional<T> try_pop() {
        lock_t lock { _mutex };

        if (_queue.empty())
            return std::nullopt;

        auto item = std::move(_queue.front());
        _queue.pop();

        return std::move(item);
    }

private:
    using lock_t = std::unique_lock<std::mutex>;

    std::mutex _mutex;
    std::queue<Item> _queue;
};

}
