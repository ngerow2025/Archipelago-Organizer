#pragma once

#include <QWidget>
#include <QNetworkAccessManager>

struct APWorldGithubRemoteInfo {
    QString name;
    QString description;
    QString owner;
};

struct ReleaseAsset {
    QString url;
    QString name;
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

   signals:
    void repoInfoReceived(const APWorldGithubRemoteInfo& info);
    void repoInfoError(const QString& error);
    void releaseProcessed(const ReleaseInfo& info);
    void allReleasesProcessed();
    void allReleases(const std::vector<ReleaseInfo>& releases);

   private:
    void getRepoInfo();
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
    QNetworkAccessManager* m_manager;
};
