#pragma once

#include <QString>
#include <unordered_map>

class SteamGame {
   public:
    SteamGame() = delete;


    const int appId;
    const std::unordered_map<QString, unsigned int> depots;
    const std::unordered_map<QString, unsigned int> branches;

    const QString preferedBranch;
    const QString preferedDepot;

    
};

