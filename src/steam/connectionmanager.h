#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <chrono>
#include <vector>

class QNetworkReply;

enum class EndpointType { WebSocket, NetFilter };

class SteamNetworkEndpoint {
   public:
    SteamNetworkEndpoint(QString ep, QString r, EndpointType t, double load,
                         std::chrono::time_point<std::chrono::system_clock> lf = {})
        : endpoint(std::move(ep)),
          realm(std::move(r)),
          type(t),
          adjusted_load(load),
          last_failed(lf) {
    }

    QString                                            endpoint;
    QString                                            realm;
    EndpointType                                       type;
    double                                             adjusted_load;
    std::chrono::time_point<std::chrono::system_clock> last_failed;
};

class ConnectionManager : public QObject {
    Q_OBJECT

   public:
    explicit ConnectionManager(QObject* parent = nullptr);
    void sslErrors(const QList<QSslError>& errors);
    void onManagerFinished(QNetworkReply* reply);
    void refreshConnectionManagerList();
    SteamNetworkEndpoint getPreferredEndpoint();
    ~ConnectionManager();

    std::chrono::time_point<std::chrono::system_clock> getLastUpdated() const {
        return last_updated;
    }
    const std::vector<SteamNetworkEndpoint>& getEndpoints() const {
        return endpoints;
    }

   signals:
    void refreshFinished(bool success);

   private:
    std::vector<SteamNetworkEndpoint>                  endpoints;
    std::chrono::time_point<std::chrono::system_clock> last_updated;

    QNetworkAccessManager manager;
    QNetworkReply*        activeReply = nullptr;
};

#endif  // NETWORKMANAGER_H
