#pragma once

#include <string>
#include <optional>


class GithubSource {
public:
    GithubSource(const std::string& owner, const std::string& repo);
    std::string getOwner() const;
    std::string getRepo() const;
    std::optional<std::string> getGameName() const;

    bool operator==(const GithubSource& other) const;
    bool operator!=(const GithubSource& other) const;

private:
    std::string owner;
    std::string repo;
    std::optional<std::string> game_name;
};