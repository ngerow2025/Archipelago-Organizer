#include "connectionmanager.h"

#include <QFile>
#include <QNetworkReply>
#include <QObject>
#include <QtNetwork>
#include <algorithm>
#include <vdf_parser.hpp>

ConnectionManager::ConnectionManager(QObject* parent) : QObject(parent) {
    connect(&manager, &QNetworkAccessManager::finished, this, &ConnectionManager::onManagerFinished);
}

void ConnectionManager::refreshConnectionManagerList() {
    if (activeReply != nullptr) {
        qDebug() << "Refresh already in flight. Ignoring request.";
        return;
    }
    QUrl      url("https://api.steampowered.com/ISteamDirectory/GetCMListForConnect/v1/");
    QUrlQuery query;
    query.addQueryItem("format", "vdf");
    url.setQuery(query);
    QNetworkRequest request(url);

    auto reply = manager.get(request);
    activeReply = reply;

    connect(reply, &QNetworkReply::sslErrors, this, &ConnectionManager::sslErrors);
    connect(reply, &QNetworkReply::errorOccurred, this, [reply](QNetworkReply::NetworkError code) {
        qDebug() << "Network error occurred:" << code << "-" << reply->errorString();
    });

    qDebug() << "Network request sent to" << url.toString();
}

void ConnectionManager::onManagerFinished(QNetworkReply* reply) {
    qDebug() << "Request finished with status code:"
             << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    QByteArray responseData = reply->readAll();

    auto root = tyti::vdf::read(responseData.begin(), responseData.end());

    bool success =
        root.attribs.find("success") != root.attribs.end() && root.attribs["success"] == "1";
    if (!success) {
        if (root.attribs.find("message") != root.attribs.end()) {
            qDebug() << "API error message:" << QString::fromStdString(root.attribs["message"]);
        } else {
            qDebug() << "API request failed without a message.";
        }
    } else {
        qDebug() << "API request succeeded.";

        last_updated = std::chrono::system_clock::now();

        auto serverlist_it = root.childs.find("serverlist");
        if (serverlist_it != root.childs.end()) {
            const auto& serverlist_node = serverlist_it->second;
            for (const auto& [index_key, endpoint_node] : serverlist_node->childs) {
                auto ep_it = endpoint_node->attribs.find("endpoint");
                if (ep_it == endpoint_node->attribs.end()) {
                    continue;
                }
                QString endpoint_val = QString::fromStdString(ep_it->second);

                auto    realm_it = endpoint_node->attribs.find("realm");
                QString realm_val = realm_it != endpoint_node->attribs.end()
                                        ? QString::fromStdString(realm_it->second)
                                        : QString();

                auto         type_it = endpoint_node->attribs.find("type");
                EndpointType type_val = EndpointType::WebSocket;
                if (type_it != endpoint_node->attribs.end()) {
                    if (type_it->second == "netfilter") {
                        type_val = EndpointType::NetFilter;
                    }
                }

                double load_val = 0.0;
                auto   wtd_load_it = endpoint_node->attribs.find("wtd_load");
                if (wtd_load_it != endpoint_node->attribs.end()) {
                    load_val = QString::fromStdString(wtd_load_it->second).toDouble();
                } else {
                    qDebug() << "wtd_load not found for endpoint:" << endpoint_val
                             << ", falling back to load";
                    auto load_it = endpoint_node->attribs.find("load");
                    if (load_it != endpoint_node->attribs.end()) {
                        load_val = QString::fromStdString(load_it->second).toDouble();
                    }
                }

                auto existing_it = std::find_if(
                    endpoints.begin(), endpoints.end(),
                    [&](const SteamNetworkEndpoint& ep) { return ep.endpoint == endpoint_val; });

                if (existing_it != endpoints.end()) {
                    existing_it->adjusted_load = load_val;
                    qDebug() << "Updated existing endpoint load:" << endpoint_val
                             << "\tload:" << load_val << "\ttype:"
                             << (type_val == EndpointType::WebSocket ? "WebSocket" : "NetFilter");
                } else {
                    endpoints.emplace_back(endpoint_val, realm_val, type_val, load_val);
                    qDebug() << "Added new endpoint:" << endpoint_val << "\tload:" << load_val
                             << "\ttype:"
                             << (type_val == EndpointType::WebSocket ? "WebSocket" : "NetFilter");
                }
            }
        }
    }

    if (reply == activeReply) {
        activeReply = nullptr;
        emit refreshFinished(success);
    }

    reply->deleteLater();
}

SteamNetworkEndpoint ConnectionManager::getPreferredEndpoint() {
    if (this->endpoints.empty()) {
        this->refreshConnectionManagerList();
    }
    if (this->endpoints.empty()) {
        return SteamNetworkEndpoint(QString(), QString(), EndpointType::WebSocket, 0.0);
    }
    auto min_it =
        std::min_element(endpoints.begin(), endpoints.end(),
                         [](const SteamNetworkEndpoint& a, const SteamNetworkEndpoint& b) {
                             return a.adjusted_load < b.adjusted_load;
                         });
    return *min_it;
}

void ConnectionManager::sslErrors(const QList<QSslError>& errors) {
    qDebug() << "SSL errors:" << errors;
}

ConnectionManager::~ConnectionManager() {
}
