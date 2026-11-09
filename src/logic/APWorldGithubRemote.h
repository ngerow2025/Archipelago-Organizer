#pragma once

#include <QWidget>
#include <QNetworkAccessManager>

#include <map>

struct APWorldGithubRemoteInfo {
    QString name;
    QString description;
    QString owner;
};

struct ReleaseAsset {
    QString url;
    QString name;
    QString digest;
};

struct ReleaseInfo {
    QString name;
    QString tag;
    QString description;
    QString publishedAt;
    std::vector<ReleaseAsset> assets;
};

class APWorldGithubRemote : public QWidget {
    Q_OBJECT

   public:
    explicit APWorldGithubRemote(QWidget* parent, QString githubUser, QString githubRepo);
    void getRepoInfo();
    std::optional<std::map<int, std::map<int, std::map<int, QList<ReleaseInfo>>>>> getSortedReleases() const {
        return sortedReleases;
    }

   signals:
    void repoInfoReceived(const APWorldGithubRemoteInfo& info);
    void repoInfoError(const QString& error);
    void releaseProcessed(const ReleaseInfo& info);
    void allReleasesProcessed();
    void allReleases(const std::vector<ReleaseInfo>& releases);

   private:
    void getReleases();
    void fetchPage(const QString& pageUrl);
    void registerRelease(const ReleaseInfo& info);
    void finished();
    void processResponse(const QByteArray& rawData);

    QString m_githubUser;
    QString m_githubRepo;
    int processedResponses = 0;
    int expectedResponses = -1;
    bool sentAllPageRequests = false;
    std::vector<ReleaseInfo> m_releases;
    std::optional<std::map<int, std::map<int, std::map<int, QList<ReleaseInfo>>>>> sortedReleases;
    QNetworkAccessManager* m_manager;
};
