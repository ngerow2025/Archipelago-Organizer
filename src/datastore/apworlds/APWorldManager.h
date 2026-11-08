#pragma once

#include <string>
#include <vector>
#include "APWorldEntry.h"

class APWorldLocalManager {
public:
    APWorldLocalManager(std::string basePath);
    void loadAPWorldEntries();
    void saveAPWorldEntries();
    ~APWorldLocalManager();
    


private:
    std::string basePath;
    std::vector<APWorldEntry> apWorldEntries;
    static constexpr const char* APWORLD_DIRECTORY = "apworlds";
    
};