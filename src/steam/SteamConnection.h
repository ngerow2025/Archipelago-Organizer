#pragma once

#include <QWebSocket>
#include <cstdint>

#include "ConnectionManager.h"
#include "enums/steammsg.h"

class SteamConnection : public QObject {
    Q_OBJECT

   public:
    explicit SteamConnection(QObject* parent = nullptr);
    void connectToEndpoint(const SteamNetworkEndpoint& endpoint);
    void sendProtobufMessage(steam::lang::EMsg emsg, const google::protobuf::Message& body, const steam::proto::CMsgProtoBufHeader& header);

   signals:
    void connected();

   private:
    void sendClientHello();

    QWebSocket* socket;
};

bool handleMulti(const QByteArray& packetBody);


