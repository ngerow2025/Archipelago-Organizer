#include "SteamConnection.h"

#include <QUrl>
#include <QtEndian>
#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 15000
#include <magic_enum/magic_enum.hpp>

#include "enums/emsg.h"
#include "protobuf/steammessages_clientserver_login.pb.h"
#include "steammessages_auth.steamclient.pb.h"


SteamConnection::SteamConnection(QObject* parent) : QObject(parent) {
}

void processPacket(const QByteArray& packet) {
    qDebug() << "Received binary message:\n\t" << packet.toHex(' ').toUpper();
    // read first 4 bytes as EMsg
    if (packet.size() < 4) {
        qWarning() << "Received message is too short to contain EMsg.";
        return;
    }
    uint32_t rawEmsg = qFromLittleEndian<uint32_t>(packet.constData());
    uint32_t emsgProtoMask = 0x80000000UL;
    uint32_t antiProtoMask = ~emsgProtoMask;

    bool           isProto = (rawEmsg & emsgProtoMask) != 0;
    steam::lang::EMsg emsg = static_cast<steam::lang::EMsg>(rawEmsg & antiProtoMask);

    qDebug() << "IsProto: " << isProto;
    qDebug() << "EMsg (masked):" << magic_enum::enum_name(emsg);

    // read next 4 bytes as header size
    if (packet.size() < 8) {
        qWarning() << "Received message is too short to contain header size.";
        return;
    }
    int32_t headerSize = qFromLittleEndian<int32_t>(packet.constData() + 4);
    qDebug() << "Header Size:" << headerSize;

    // deserialize header
    steam::proto::CMsgProtoBufHeader header;
    if (!header.ParseFromArray(packet.constData() + 8, headerSize)) {
        qWarning() << "Failed to parse header.";
        return;
    }

    if (packet.size() < 8 + headerSize) {
        qWarning() << "Received message is too short to contain body.";
        return;
    }

    QByteArray body = packet.mid(8 + headerSize);

    if (emsg == steam::lang::EMsg::Multi) {
        if (handleMulti(body)) {
            return;
        }
    } else if (emsg == steam::lang::EMsg::ServiceMethodResponse) {
        qDebug() << "Received ServiceMethodResponse.";
        qDebug() << "Header: ";
        qDebug() << "\tTargetJobName: " << QString::fromStdString(header.target_job_name());
        qDebug() << "\tSourceJobID: " << header.jobid_source();
        qDebug() << "\tTargetJobID: " << header.jobid_target();
        qDebug() << "\tseq_num: " << header.seq_num();
        qDebug() << "\teresult: " << header.eresult();

        steam::proto::CAuthentication_BeginAuthSessionViaQR_Response response;
        if (!response.ParseFromArray(body.constData(), body.size())) {
            qWarning() << "Failed to parse ServiceMethodResponse body.";
        } else {
            qDebug() << "Parsed ServiceMethodResponse body successfully.";
            // response.set_
            qDebug() << "challenge URL: " << QString::fromStdString(response.challenge_url());
            qDebug() << "polling interval: " << response.interval();
            qDebug() << "allowed_confirmations: ";
            for (const auto& confirmation : response.allowed_confirmations()) {
                qDebug() << "\tconfirmation: " << magic_enum::enum_name(confirmation.confirmation_type());
                qDebug() << "\tassociated message:" << QString::fromStdString(confirmation.associated_message());
            }
        }

    } else {
        qDebug() << "unknown message with EMsg:" << magic_enum::enum_name(emsg);
    }
}

bool handleMulti(const QByteArray& packetBody) {
    // parse the remaining message as a CMsgMulti
    // then if size_zipped > 0 then decompress the payload
    // then parse the payload as packed packets, each is started with a 4 byte size
    // pass that back into this function to be processed as normal
    steam::proto::CMsgMulti multi;
    if (!multi.ParseFromArray(packetBody.constData(), packetBody.size())) {
        qWarning() << "Failed to parse CMsgMulti.";
        return true;
    }
    // check if size_unzipped > 0
    if (multi.size_unzipped() > 0) {
        qDebug() << "CMsgMulti has unzipped size > 0, decompressing payload.";
    } else {
        qDebug() << "CMsgMulti has unzipped size 0, processing payload as is.";
        QByteArray payload =
            QByteArray::fromRawData(multi.message_body().data(), multi.message_body().size());
        // process the payload as packed packets
        while (payload.size() > 0) {
            if (payload.size() < 4) {
                qWarning() << "CMsgMulti payload is too short to contain packet size.";
                return true;
            }
            int32_t packetSize = qFromLittleEndian<int32_t>(payload.data());
            if (payload.size() < 4 + packetSize) {
                qWarning() << "CMsgMulti payload is too short to contain full packet.";
                return true;
            }
            QByteArray packet = payload.mid(4, packetSize);
            // call this function recursively with the new packet
            processPacket(packet);
            // remove the processed packet from the payload
            payload.remove(0, 4 + packetSize);
        }
    }
    return false;
}

void SteamConnection::connectToEndpoint(const SteamNetworkEndpoint& endpoint) {
    QUrl connectionUrl(QString("wss://%1/cmsocket/").arg(endpoint.endpoint));

    socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    // do connect calls here
    // connect logging function for binaryMessageReceived
    connect(socket, &QWebSocket::binaryMessageReceived, this,
            [this](const QByteArray& message) { processPacket(message); });
    connect(socket, &QWebSocket::connected, this, [this]() {
        sendClientHello();
        emit connected();
    });

    socket->open(connectionUrl);
}

void SteamConnection::sendProtobufMessage(steam::lang::EMsg                   emsg,
                                          const google::protobuf::Message& body,
                                          const steam::proto::CMsgProtoBufHeader&  header) {
    // 1. Get the exact byte sizes of the protobuf components
    const int32_t headerSize = static_cast<int32_t>(header.ByteSizeLong());
    const int32_t bodySize = static_cast<int32_t>(body.ByteSizeLong());
    // 2. Compute total packet size: 4 bytes (EMsg) + 4 bytes (header size) + header + body
    const int32_t totalSize = sizeof(int32_t) + sizeof(int32_t) + headerSize + bodySize;
    // 3. Pre-allocate the exact contiguous block in QByteArray
    QByteArray packet;
    packet.resize(totalSize);
    char* ptr = packet.data();
    // 4. Write headers in Little Endian
    uint32_t protoMask = 0x80000000;
    int32_t  maskedEMsg = static_cast<int32_t>(emsg) | protoMask;
    qToLittleEndian(maskedEMsg, ptr);
    ptr += sizeof(int32_t);
    qToLittleEndian(headerSize, ptr);
    ptr += sizeof(int32_t);
    // 5. Serialize protobuf messages directly into the pre-allocated QByteArray buffer
    if (!header.SerializeToArray(ptr, headerSize)) {
        qWarning() << "Failed to serialize header.";
    }
    ptr += headerSize;
    if (!body.SerializeToArray(ptr, bodySize)) {
        qWarning() << "Failed to serialize body.";
    }
    qDebug() << "Sending Protobuf Message:\n\t" << packet.toHex(' ').toUpper();
    socket->sendBinaryMessage(packet);
}

void SteamConnection::sendClientHello() {
    steam::proto::CMsgClientHello helloMessage{};
    helloMessage.set_protocol_version(steam::lang::MsgClientLogon::CurrentProtocol);
    sendProtobufMessage(steam::lang::EMsg::ClientHello, helloMessage, steam::proto::CMsgProtoBufHeader{});
}