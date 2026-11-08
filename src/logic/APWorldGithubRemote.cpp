#include "APWorldGithubRemote.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

APWorldGithubRemote::APWorldGithubRemote(QWidget* parent, QString githubUser, QString githubRepo)
    : QWidget(parent), m_githubUser(githubUser), m_githubRepo(githubRepo), sortedReleases(std::nullopt) {
    connect(this, &APWorldGithubRemote::repoInfoReceived, this,
            [this](const APWorldGithubRemoteInfo& info) { this->getReleases(); });

    connect(this, &APWorldGithubRemote::allReleases, this, [this](const auto& releases) {
        qDebug() << "All releases processed.";
        qDebug() << "# of total releases:" << releases.size();
    });
    qDebug() << "APWorldGithubRemote initialized for user:" << githubUser << ", repo:" << githubRepo;
    m_manager = new QNetworkAccessManager(this);
};

void APWorldGithubRemote::getRepoInfo() {
    auto url = QString("https://api.github.com/repos/%1/%2").arg(m_githubUser, m_githubRepo);
    auto request = QNetworkRequest(QUrl(url));
    request.setRawHeader("X-GitHub-Api-Version", "2026-03-10");
    auto* reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        qDebug() << "Repo info received: " << reply->url().toString();

        if (reply->error() == QNetworkReply::ContentNotFoundError) {
            qDebug() << "Repo not found or is private.";
            qDebug() << "url:" << reply->url().toString();
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error: " << reply->errorString();
            return;
        }
        auto            rawData = reply->readAll();
        QJsonParseError parseError;
        QJsonObject     jsonDoc = QJsonDocument::fromJson(rawData, &parseError).object();
        if (parseError.error != QJsonParseError::NoError) {
            emit repoInfoError("JSON parse error" + parseError.errorString());
            return;
        }

        APWorldGithubRemoteInfo info;

        info.description = jsonDoc["description"].toString();
        info.name = jsonDoc["name"].toString();
        info.owner = jsonDoc["owner"][QString("login")].toString();
        emit repoInfoReceived(info);
    });
}

void APWorldGithubRemote::fetchPage(const QString& pageUrl) {
    auto request = QNetworkRequest(QUrl(pageUrl));
    request.setRawHeader("X-GitHub-Api-Version", "2026-03-10");
    auto* reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        qDebug() << "Recieved: " << reply->url().toString();
        if (reply->error() == QNetworkReply::ContentNotFoundError) {
            qDebug() << "Releases not found or are private.";
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error: " << reply->errorString();
            return;
        }

        auto rawData = reply->readAll();
        processResponse(rawData);

        auto linkHeader = reply->rawHeader("link");
        // look for the entry that has rel=\"next\"
        QString nextPageUrl;
        for (const QString& part : linkHeader.split(',')) {
            if (part.contains("rel=\"next\"")) {
                int start = part.indexOf('<') + 1;
                int end = part.indexOf('>');
                if (start > 0 && end > start) {
                    nextPageUrl = part.mid(start, end - start);
                }
                break;
            }
        }
        qDebug() << "next page URL:" << nextPageUrl;

        if (!nextPageUrl.isEmpty()) {
            fetchPage(nextPageUrl);
        } else {
            qDebug() << "No more pages to fetch.";
            finished();
        }
    });
}

void APWorldGithubRemote::getReleases() {
    auto url = QString("https://api.github.com/repos/%1/%2/releases?per_page=100")
                   .arg(m_githubUser, m_githubRepo);
    fetchPage(url);
}

void APWorldGithubRemote::registerRelease(const ReleaseInfo& info) {
    m_releases.push_back(info);
}

void APWorldGithubRemote::processResponse(const QByteArray& rawData) {
    QJsonParseError parseError;
    QJsonArray      jsonDoc = QJsonDocument::fromJson(rawData, &parseError).array();
    if (parseError.error != QJsonParseError::NoError) {
        emit repoInfoError("JSON parse error" + parseError.errorString());
        return;
    }

    // Process the releases data
    for (const QJsonValue& value : jsonDoc) {
        QJsonObject release = value.toObject();
        qDebug() << "Processing release: " << release;
        // Process each release
        // qDebug() << "Release: " << release["name"].toString();
        // qDebug() << "Description: " << release["body"].toString();
        // qDebug() << "Published: " << release["published_at"].toString();
        ReleaseInfo releaseInfo;
        releaseInfo.name = release["name"].toString();
        releaseInfo.tag = release["tag_name"].toString();
        releaseInfo.description = release["body"].toString();
        releaseInfo.publishedAt = release["published_at"].toString();
        for (const auto& asset : release["assets"].toArray()) {
            ReleaseAsset assetInfo;
            assetInfo.url = asset.toObject()["url"].toString();
            assetInfo.name = asset.toObject()["name"].toString();
            releaseInfo.assets.push_back(assetInfo);
        }
        emit releaseProcessed(releaseInfo);
        registerRelease(releaseInfo);
    }
}

void APWorldGithubRemote::finished() {
    
    for (const ReleaseInfo& release : m_releases) {
        qDebug() << "Release: " << release.name << ", Published: " << release.publishedAt;
        for (const ReleaseAsset& asset : release.assets) {
            qDebug() << "  Asset: " << asset.name << ", URL: " << asset.url;
        }
    }

    sortedReleases = std::map<int, std::map<int, std::map<int, QList<ReleaseInfo>>>>();

    //sort into major, minor, and patch releases
    for (const auto release : m_releases) {
        QStringList versionParts = release.tag.split('.');
        if (versionParts.size() >= 3) {
            int major = versionParts[0].toInt();
            int minor = versionParts[1].toInt();
            int patch = versionParts[2].toInt();
            
            sortedReleases.value()[major][minor][patch].push_back(release);
        }
    }

    // Print sorted releases for debugging
    for (const auto& [major_version, minor_versions] : sortedReleases.value()) {
        for (const auto& [minor_version, patch_versions] : minor_versions) {
            for (const auto& [patch_version, releases] : patch_versions) {
                for (const ReleaseInfo& release : releases) {
                    qDebug() << "Release: " << major_version << "." << minor_version << "." << patch_version << " - " << release.name << ", Published: " << release.publishedAt;
                }
            }
        }
    }

    emit allReleasesProcessed();
    emit allReleases(m_releases);
}