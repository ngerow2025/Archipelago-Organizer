#ifndef CONNECTION_H
#define CONNECTION_H

#include <QWebSocket>
#include <cstdint>

#include "connectionmanager.h"
#include "enums/steammsg.h"

class SteamConnection : public QObject {
    Q_OBJECT

   public:
    explicit SteamConnection(QObject* parent = nullptr);
    void connectToEndpoint(const SteamNetworkEndpoint& endpoint);
    void sendProtobufMessage(SteamKit::EMsg emsg, const google::protobuf::Message& body, const CMsgProtoBufHeader& header);

   signals:
    void connected();

   private:
    void sendClientHello();

    QWebSocket* socket;
};

bool handleMulti(const QByteArray& packetBody);

#endif  // CONNECTION_H

