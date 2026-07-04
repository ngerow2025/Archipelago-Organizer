#include "SteamAuthConnection.h"

#include <QDateTime>
#include <QTimeZone>
#include <QHostInfo>

#include "JobID.h"
#include "steammessages_auth.steamclient.pb.h"
#include "Util.h"



SteamAuthConnection::SteamAuthConnection(QObject* parent)
    : QObject(parent) {
    rawConnection = new SteamConnection(this);
    connect(rawConnection, &SteamConnection::connected, this, [this](){
        emit unauthConnected();
    });

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QDateTime referenceDateTime = QDateTime(QDate(2005, 1, 1), QTime(0, 0, 0), QTimeZone::utc());

    processTime = static_cast<uint64_t>(referenceDateTime.secsTo(currentDateTime));
}

void SteamAuthConnection::connectToEndpoint(const SteamNetworkEndpoint& endpoint) {
    rawConnection->connectToEndpoint(endpoint);
}


void SteamAuthConnection::beginAuth() {
    steam::proto::CAuthentication_BeginAuthSessionViaQR_Request request{};
    request.set_website_id("Client");
    request.mutable_device_details()->set_device_friendly_name(
        (QHostInfo::localHostName() + " (Archipelago Organiser)").toStdString());
    request.mutable_device_details()->set_platform_type(
        steam::proto::EAuthTokenPlatformType::k_EAuthTokenPlatformType_SteamClient);
    request.mutable_device_details()->set_os_type((int32_t)GetOSType());

    sendNonAuthenticatedServiceMethodCall("Authentication.BeginAuthSessionViaQR#1", request);
}

void SteamAuthConnection::sendNonAuthenticatedServiceMethodCall(
    const std::string& method, const google::protobuf::Message& body) {

    auto               emsg = steam::lang::EMsg::ServiceMethodCallFromClientNonAuthed;
    
    steam::proto::CMsgProtoBufHeader header{};
    JobID              jobID = JobID::createClientJobID(processTime);
    header.set_jobid_source(jobID.getRawJobID());
    header.set_target_job_name(method);

    qDebug() << "Sending non-authenticated service method call:";
    qDebug() << "\tMethod: " << QString::fromStdString(method);
    qDebug() << "\tJobID: " << jobID.getRawJobID();

    rawConnection->sendProtobufMessage(emsg, body, header);
}
