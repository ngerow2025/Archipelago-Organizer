#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <QList>

/**
 * @struct SteamLibrary
 * @brief Represents a Steam library entry in the database
 */
struct SteamLibrary {
    int id = -1;
    QString path;

    SteamLibrary() = default;
    SteamLibrary(int id, const QString &path) : id(id), path(path) {}

    bool isValid() const {
        return id > 0 && !path.isEmpty();
    }
};

/**
 * @struct Game
 * @brief Represents a game entry in the database
 */
struct Game {
    int id = -1;
    QString name;
    QString path;

    Game() = default;
    Game(int id, const QString &name, const QString &path)
        : id(id), name(name), path(path) {}

    bool isValid() const {
        return id > 0 && !name.isEmpty() && !path.isEmpty();
    }
};

#endif // DATATYPES_H
