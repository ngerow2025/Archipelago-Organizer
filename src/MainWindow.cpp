#include "MainWindow.h"

#include "datastore/Database.h"
#include "enums/emsg.h"
#include "enums/steammsg.h"
#include "frontend/ap_worlds/APWorldWidget.h"
#include "frontend/games/GameTabWidget.h"
#include "protobuf/steammessages_clientserver_login.pb.h"
#include "steam/ConnectionManager.h"
#include "steam/SteamAuthConnection.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    createUI();
    // new Database(this);
    {
        // auto nm = new ConnectionManager(this);
        // connect(nm, &ConnectionManager::refreshFinished, this, [this, nm](bool success) {
        //     if (!success) {
        //         qDebug() << "Failed to refresh connection manager list.";
        //         return;
        //     }
        //     auto preferredEndpoint = nm->getPreferredEndpoint();
        //     auto authConnection = new SteamAuthConnection(this);
        //     connect(authConnection, &SteamAuthConnection::unauthConnected, this, [this, authConnection]() {
        //         qDebug() << "Successfully connected to Steam Auth Service.";
        //         authConnection->beginAuth();
        //     });
        //     authConnection->connectToEndpoint(preferredEndpoint);
        // });
        // nm->refreshConnectionManagerList();
    }
}

MainWindow::~MainWindow() {
}

void MainWindow::createUI() {
    tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("tabWidget");

    gamesTab = new GameTabWidget(this);
    gamesTab->setObjectName("gamesTab");
    tabWidget->addTab(gamesTab, "Games");
    APWorldWidget* apWorldWidget = new APWorldWidget(this);
    tabWidget->addTab(apWorldWidget, "APWorlds");
    tabWidget->addTab(new QWidget(this), "Player YAMLs");

    setCentralWidget(tabWidget);
}