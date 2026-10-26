#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPair>
#include <functional>

class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject *parent = nullptr) : QObject(parent) {}

    void setBearerToken(const QString &token);
    QString bearerToken() const { return m_bearerToken; }
    void clearToken() { m_bearerToken.clear(); }

    void get(const QUrl &url,
             const std::function<void(const QByteArray&)> &onSuccess,
             const std::function<void(const QString&)> &onError = nullptr);

    // Generic POST request
    void post(const QUrl &url,
              const QByteArray &data,
              const std::function<void(const QByteArray&)> &onSuccess,
              const std::function<void(const QString&)> &onError = nullptr);

    // Convenient POST method with QVariantMap (auto-converts to JSON)
    void postJson(const QUrl &url,
                  const QVariantMap &data,
                  const std::function<void(const QByteArray&)> &onSuccess,
                  const std::function<void(const QString&)> &onError = nullptr);

    // Convenient POST method with QJsonObject
    void postJson(const QUrl &url,
                  const QJsonObject &data,
                  const std::function<void(const QByteArray&)> &onSuccess,
                  const std::function<void(const QString&)> &onError = nullptr);

    void getStatus();

private:
    QNetworkAccessManager manager;
    QPair<QString, QString> m_statusResult;
    QString m_bearerToken;

    void addAuthHeaders(QNetworkRequest &request);

signals:
    void statusReady(const QPair<QString, QString> &result);
    void statusError(const QString &error);

    void postResponseReceived(const QByteArray &response);
    void postError(const QString &error);
};

#endif // APICLIENT_H
