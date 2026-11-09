#pragma once

#include <string>
#include <vector>
#include "APWorldEntry.h"
#include "GithubSource.h"

class APWorldLocalManager {
public:
    APWorldLocalManager(std::string basePath);
    void load();
    void save();
    void installAPWorldFromDiskFile(const std::string& filePath);
    void installAPWorldFromDiskDirectory(const std::string& directoryPath);
    void installAPWorldFromMemory(const std::vector<char>& buffer);
    void registerGithubSource(const GithubSource& source);
    void installAPWorldFromUrl(const std::string& url);
    void installAPWorldFromGithubRelease(const GithubSource& source, const std::string& releaseTag);
    ~APWorldLocalManager();
    


private:
    std::string basePath;
    std::vector<APWorldEntry> apWorldEntries;
    std::vector<GithubSource> githubSources;
    static constexpr const char* APWORLD_DIRECTORY = "apworlds";
    
};