#ifndef SYNC_HPP
#define SYNC_HPP

#include <queue>
#include <mutex>

namespace sync {

template <class T>
class Queue {
public:
    using Item = T;

    void push(Item && item) {
        lock_t lock { _mutex };

        _queue.push(std::move(item));
    }

    bool empty() {
        lock_t lock { _mutex };

        return _queue.empty();
    }

    T pop() {
        lock_t lock { _mutex };

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

#endif // SYNC_HPP
