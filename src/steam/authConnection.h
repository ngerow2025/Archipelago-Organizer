#ifndef AUTHCONNECTION_H
#define AUTHCONNECTION_H

#include <QObject>

#include "connection.h"
#include "connectionmanager.h"

class SteamAuthConnection : public QObject {
    Q_OBJECT

   public:
    explicit SteamAuthConnection(QObject* parent = nullptr);
    void connectToEndpoint(const SteamNetworkEndpoint& endpoint);
    void beginAuth();

   signals:
    void unauthConnected();
    void connected();

   private:
    void sendAuthRequest();
    void sendNonAuthenticatedServiceMethodCall(const std::string&               method,
                                               const google::protobuf::Message& body);

    SteamConnection* rawConnection;
    // seconds since Jan 1, 2005
    uint64_t processTime;
};

#endif  // AUTHCONNECTION_H