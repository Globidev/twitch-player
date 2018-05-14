#pragma once

#include <memory>
#include <future>

#include <QtPromise>

struct Cancellable {
    void cancel() {
        _cancelled = true;
    }

    bool cancelled() const {
        return _cancelled;
    }
private:
    bool _cancelled = false;
};

struct CancelError: std::future_error {
    CancelError(): std::future_error { std::future_errc::broken_promise } { }
};

template <class T>
auto make_cancellable(QtPromise::QPromise<T> promise) {
    auto cancel_token = std::make_shared<Cancellable>();

    auto tapped_promise = promise.tap([=](auto...) {
        if (cancel_token->cancelled())
            throw CancelError { };
    });

    return std::make_pair(cancel_token, tapped_promise);
}

using CancelToken = std::shared_ptr<Cancellable>;
