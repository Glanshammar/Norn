#include "apiclient.h"
#include <QThread>

void ApiClient::setBearerToken(const QString &token) {
    m_bearerToken = token;
    if (!m_bearerToken.isEmpty()) {
        qDebug() << "Bearer token set (length:" << m_bearerToken.length() << ")";
    } else {
        qDebug() << "Bearer token cleared";
    }
}

void ApiClient::addAuthHeaders(QNetworkRequest &request) {
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);

    if (!m_bearerToken.isEmpty()) {
        QString authHeader = "Bearer " + m_bearerToken;
        request.setRawHeader("Authorization", authHeader.toUtf8());
        qDebug() << "Added Authorization header";
    }
}

void ApiClient::get(const QUrl &url,
                    const std::function<void(const QByteArray&)> &onSuccess,
                    const std::function<void(const QString&)> &onError){

    QNetworkRequest request(url);
    addAuthHeaders(request);

    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            if (onSuccess) onSuccess(reply->readAll());
        } else {
            if (onError) onError(reply->errorString());
        }
        reply->deleteLater();
    });
}

void ApiClient::getStatus(){
    get(QUrl("http://127.0.0.1:5285/api/status"),
            [this](const QByteArray &data) {
                qDebug() << "Got data:" << data;

                QJsonParseError parseError;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);

                if (parseError.error != QJsonParseError::NoError) {
                    qDebug() << "getStatus: JSON parse error, emitting statusError";
                    emit statusError("JSON parse failed");
                    return;
                }

                if (!jsonDoc.isObject()) {
                    qDebug() << "getStatus: JSON is not an object, emitting statusError";
                    emit statusError("JSON is not an object");
                    return;
                }

                QJsonObject jsonObj = jsonDoc.object();
                QString status = jsonObj["status"].toString();
                QString message = jsonObj["message"].toString();

                qDebug() << "getStatus: Success, emitting statusReady with:" << status << message;
                m_statusResult = qMakePair(status, message);  // Store in member

                QMetaObject::invokeMethod(this, [this]() {
                    emit statusReady(m_statusResult);
                });
            },
            [this](const QString &error) {
                qDebug() << "getStatus: Network error, emitting statusError:" << error;
                QMetaObject::invokeMethod(this, [this, error]() {
                    emit statusError(error);
                });
            });
}

// ================ POST Method ================

void ApiClient::post(const QUrl &url,
                     const QByteArray &data,
                     const std::function<void(const QByteArray&)> &onSuccess,
                     const std::function<void(const QString&)> &onError) {

    QNetworkRequest request(url);
    addAuthHeaders(request);

    qDebug() << "POST Request to:" << url.toString();
    qDebug() << "POST Data:" << data;

    QNetworkReply *reply = manager.post(request, data);

    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "POST Response received, size:" << response.size();
            qDebug() << "POST Response:" << response;

            emit postResponseReceived(response);

            if (onSuccess) onSuccess(response);
        } else {
            QString error = QString("Network Error (%1): %2")
            .arg(reply->error())
                .arg(reply->errorString());
            qDebug() << "POST Error:" << error;

            emit postError(error);

            if (onError) onError(error);
        }
        reply->deleteLater();
    });

    connect(reply, &QNetworkReply::sslErrors, [=](const QList<QSslError> &errors) {
        qDebug() << "SSL Errors in POST:";
        for (const auto &error : errors) {
            qDebug() << "  -" << error.errorString();
        }
    });

    request.setTransferTimeout(10000); // 10 seconds
}

// ================ POST with QVariantMap ================

void ApiClient::postJson(const QUrl &url,
                         const QVariantMap &data,
                         const std::function<void(const QByteArray&)> &onSuccess,
                         const std::function<void(const QString&)> &onError) {

    QJsonObject jsonObj = QJsonObject::fromVariantMap(data);
    QJsonDocument jsonDoc(jsonObj);
    QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);

    post(url, jsonData, onSuccess, onError);
}

// ================ POST with QJsonObject ================

void ApiClient::postJson(const QUrl &url,
                         const QJsonObject &data,
                         const std::function<void(const QByteArray&)> &onSuccess,
                         const std::function<void(const QString&)> &onError) {

    QJsonDocument jsonDoc(data);
    QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);

    post(url, jsonData, onSuccess, onError);
}
