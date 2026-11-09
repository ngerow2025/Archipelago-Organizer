#include "GithubSource.h"


GithubSource::GithubSource(const std::string& owner, const std::string& repo)
    : owner(owner), repo(repo), game_name(std::nullopt) {}

std::string GithubSource::getOwner() const {
    return owner;
}

std::string GithubSource::getRepo() const {
    return repo;
}

std::optional<std::string> GithubSource::getGameName() const {
    return game_name;
}