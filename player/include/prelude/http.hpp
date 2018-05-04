#ifndef HTTP_HPP
#define HTTP_HPP

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <functional>

template <class T>
class ResponseFuture {
public:
    template <class F>
    ResponseFuture(QNetworkReply *reply, F parse):
        _reply(reply), _parse(parse)
    {
        _reply->ignoreSslErrors();

        QObject::connect(_reply, &QNetworkReply::finished, [=] {
            if (_reply->isOpen() && _on_complete)
                _on_complete(_parse(_reply->readAll()));
        });

        using reply_error_t = void (QNetworkReply::*)(QNetworkReply::NetworkError);
        auto reply_error = static_cast<reply_error_t>(&QNetworkReply::error);
        QObject::connect(_reply, reply_error, [=](auto error) {
            if (_on_error)
                _on_error(error);
        });
    }

    ~ResponseFuture() {
        _reply->deleteLater();
    }

    void cancel() {
        _reply->abort();
    }

    template <class F> auto then(F f)  { _on_complete = f;  return this; }
    template <class F> auto error(F f) { _on_error = f;     return this; }

private:
    using parser_t = std::function<T (const QByteArray &)>;
    using on_completion_t = std::function<void (T)>;
    using on_error_t = std::function<void (QNetworkReply::NetworkError)>;

    QNetworkReply *_reply;
    parser_t _parse;

    on_completion_t _on_complete;
    on_error_t _on_error;
};

class APIClient {
public:
    template <class F>
    auto get(const QNetworkRequest &request, F parser) {
        using T = std::result_of_t<F(const QByteArray &)>;

        auto reply = _http_client.get(request);
        return std::make_unique<ResponseFuture<T>>(reply, parser);
    }

protected:
    template <class T>
    using response_t = std::unique_ptr<ResponseFuture<T>>;

private:
    QNetworkAccessManager _http_client;
};

#endif // HTTP_HPP
