#ifndef DISK_GAME_H
#define DISK_GAME_H

#include <QString>

class DiskGame {
   public:
    DiskGame() = delete;
    DiskGame(const QString& path);
    ~DiskGame() = default;

};

#endif  // DISK_GAME_H