#include "mainwindow.h"
#include "frontend/games/gametabwidget.h"
#include "datastore/database.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createUI();
    new Database(this);
}

MainWindow::~MainWindow()
{
}

void MainWindow::createUI()
{
    tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("tabWidget");
    
    gamesTab = new GameTabWidget(this);
    gamesTab->setObjectName("gamesTab");
    tabWidget->addTab(gamesTab, "Games");

    setCentralWidget(tabWidget);
}