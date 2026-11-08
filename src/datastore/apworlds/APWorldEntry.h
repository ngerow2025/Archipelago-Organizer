#pragma once

#include <string>



enum class LocalSourceType {
    APWorldFile,
    Directory,
    NotLocal,
};

enum class RemoteSourceType {
        GithubRelease,
        ArchipelagoMWGithub,
        NotRemote,
};


class APWorldEntry {
public:
    explicit APWorldEntry(int majorVersion, int minorVersion, int patchVersion,
                          std::string game, std::string remoteUrl, std::string localPath,
                          LocalSourceType localSourceType, RemoteSourceType remoteSourceType)
        : majorVersion(majorVersion),
          minorVersion(minorVersion),
          patchVersion(patchVersion),
          game(std::move(game)),
          remoteUrl(std::move(remoteUrl)),
          localPath(std::move(localPath)),
          localSourceType(localSourceType),
          remoteSourceType(remoteSourceType) {}
    int majorVersion;
    int minorVersion;
    int patchVersion;
    std::string game;
    std::string remoteUrl;
    std::string localPath;
    LocalSourceType localSourceType;
    RemoteSourceType remoteSourceType;
};
