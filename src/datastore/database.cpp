#include "database.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

Database::Database(QObject* parent) : QObject(parent) {
    databaseConnection = QSqlDatabase::addDatabase("QSQLITE");
    databaseConnection.setDatabaseName("archipelago_organiser.db");
    if (!databaseConnection.open()) {
        qWarning() << "Failed to open database:" << databaseConnection.lastError().text();
    } else {
        qDebug() << "Database opened successfully.";
    }

    setupDatabase();
}

Database::~Database() {
}

void Database::setupDatabase() {
    createSteamLibraryTable();
    checkDatabaseError();
    createGameTable();
    checkDatabaseError();
}

void Database::checkDatabaseError() {
    auto error = databaseConnection.lastError();
    if (error.type() != QSqlError::NoError) {
        qWarning() << "Database error:" << error.text();
    }
}

void Database::createSteamLibraryTable() {
    QSqlQuery query(databaseConnection);
    QString   createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS steam_library (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            path TEXT NOT NULL
        )
    )";
    if (!query.exec(createTableSQL)) {
        qWarning() << "Failed to create steam_library table:" << query.lastError().text();
    } else {
        qDebug() << "steam_library table created or already exists.";
    }
}

void Database::createGameTable() {
    QSqlQuery query(databaseConnection);
    QString   createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS game (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            path TEXT NOT NULL
        )
    )";
    if (!query.exec(createTableSQL)) {
        qWarning() << "Failed to create game table:" << query.lastError().text();
    } else {
        qDebug() << "game table created or already exists.";
    }
}

// ============================================================================
// CRUD METHODS FOR STEAM LIBRARY
// ============================================================================

int Database::addSteamLibrary(const QString& path) {
    if (path.isEmpty()) {
        qWarning() << "Cannot add SteamLibrary with empty path";
        return -1;
    }

    QSqlQuery query(databaseConnection);
    query.prepare("INSERT INTO steam_library (path) VALUES (:path)");
    query.addBindValue(path);

    if (!query.exec()) {
        qWarning() << "Failed to insert SteamLibrary:" << query.lastError().text();
        return -1;
    }

    int id = query.lastInsertId().toInt();
    qDebug() << "SteamLibrary added with id:" << id;
    return id;
}

SteamLibrary Database::getSteamLibraryById(int id) {
    if (id <= 0) {
        qWarning() << "Invalid id provided to getSteamLibraryById";
        return SteamLibrary();
    }

    QSqlQuery query(databaseConnection);
    query.prepare("SELECT id, path FROM steam_library WHERE id = :id");
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to retrieve SteamLibrary:" << query.lastError().text();
        return SteamLibrary();
    }

    if (query.next()) {
        return SteamLibrary(query.value(0).toInt(), query.value(1).toString());
    }

    qDebug() << "SteamLibrary with id" << id << "not found";
    return SteamLibrary();
}

QList<SteamLibrary> Database::getAllSteamLibraries() {
    QList<SteamLibrary> libraries;
    QSqlQuery           query(databaseConnection);

    if (!query.exec("SELECT id, path FROM steam_library")) {
        qWarning() << "Failed to retrieve all SteamLibraries:" << query.lastError().text();
        return libraries;
    }

    while (query.next()) {
        libraries.append(SteamLibrary(query.value(0).toInt(), query.value(1).toString()));
    }

    qDebug() << "Retrieved" << libraries.size() << "SteamLibraries";
    return libraries;
}

bool Database::updateSteamLibrary(int id, const QString& path) {
    if (id <= 0 || path.isEmpty()) {
        qWarning() << "Invalid parameters for updateSteamLibrary";
        return false;
    }

    QSqlQuery query(databaseConnection);
    query.prepare("UPDATE steam_library SET path = :path WHERE id = :id");
    query.addBindValue(path);
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to update SteamLibrary:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qDebug() << "SteamLibrary with id" << id << "updated";
        return true;
    }

    qDebug() << "No SteamLibrary found with id" << id << "to update";
    return false;
}

bool Database::deleteSteamLibrary(int id) {
    if (id <= 0) {
        qWarning() << "Invalid id provided to deleteSteamLibrary";
        return false;
    }

    QSqlQuery query(databaseConnection);
    query.prepare("DELETE FROM steam_library WHERE id = :id");
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to delete SteamLibrary:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qDebug() << "SteamLibrary with id" << id << "deleted";
        return true;
    }

    qDebug() << "No SteamLibrary found with id" << id << "to delete";
    return false;
}

// ============================================================================
// CRUD METHODS FOR GAME
// ============================================================================

int Database::addGame(const QString& name, const QString& path) {
    if (name.isEmpty() || path.isEmpty()) {
        qWarning() << "Cannot add Game with empty name or path";
        return -1;
    }

    QSqlQuery query(databaseConnection);
    query.prepare("INSERT INTO game (name, path) VALUES (:name, :path)");
    query.addBindValue(name);
    query.addBindValue(path);

    if (!query.exec()) {
        qWarning() << "Failed to insert Game:" << query.lastError().text();
        return -1;
    }

    int id = query.lastInsertId().toInt();
    qDebug() << "Game added with id:" << id;
    return id;
}

Game Database::getGameById(int id) {
    if (id <= 0) {
        qWarning() << "Invalid id provided to getGameById";
        return Game();
    }

    QSqlQuery query(databaseConnection);
    query.prepare("SELECT id, name, path FROM game WHERE id = :id");
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to retrieve Game:" << query.lastError().text();
        return Game();
    }

    if (query.next()) {
        return Game(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString());
    }

    qDebug() << "Game with id" << id << "not found";
    return Game();
}

QList<Game> Database::getAllGames() {
    QList<Game> games;
    QSqlQuery   query(databaseConnection);

    if (!query.exec("SELECT id, name, path FROM game")) {
        qWarning() << "Failed to retrieve all Games:" << query.lastError().text();
        return games;
    }

    while (query.next()) {
        games.append(
            Game(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString()));
    }

    qDebug() << "Retrieved" << games.size() << "Games";
    return games;
}

bool Database::updateGame(int id, const QString& name, const QString& path) {
    if (id <= 0 || name.isEmpty() || path.isEmpty()) {
        qWarning() << "Invalid parameters for updateGame";
        return false;
    }

    QSqlQuery query(databaseConnection);
    query.prepare("UPDATE game SET name = :name, path = :path WHERE id = :id");
    query.addBindValue(name);
    query.addBindValue(path);
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to update Game:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qDebug() << "Game with id" << id << "updated";
        return true;
    }

    qDebug() << "No Game found with id" << id << "to update";
    return false;
}

bool Database::deleteGame(int id) {
    if (id <= 0) {
        qWarning() << "Invalid id provided to deleteGame";
        return false;
    }

    QSqlQuery query(databaseConnection);
    query.prepare("DELETE FROM game WHERE id = :id");
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to delete Game:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qDebug() << "Game with id" << id << "deleted";
        return true;
    }

    qDebug() << "No Game found with id" << id << "to delete";
    return false;
}