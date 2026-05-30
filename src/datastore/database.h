#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlRelationalTableModel>
#include <QSqlDatabase>
#include <QList>
#include "games/datatypes.h"


class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

signals:

public slots:

    // CRUD methods for SteamLibrary
    int addSteamLibrary(const QString &path);
    SteamLibrary getSteamLibraryById(int id);
    QList<SteamLibrary> getAllSteamLibraries();
    bool updateSteamLibrary(int id, const QString &path);
    bool deleteSteamLibrary(int id);

    // CRUD methods for Game
    int addGame(const QString &name, const QString &path);
    Game getGameById(int id);
    QList<Game> getAllGames();
    bool updateGame(int id, const QString &name, const QString &path);
    bool deleteGame(int id);

private:
    QSqlRelationalTableModel *databaseModel;
    QSqlDatabase databaseConnection;

    void setupDatabase();
    void checkDatabaseError();

    void createSteamLibraryTable();
    void createGameTable();
};

#endif // DATABASE_H
