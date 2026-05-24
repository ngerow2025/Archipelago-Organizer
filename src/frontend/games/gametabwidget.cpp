#include "gametabwidget.h"
#include "steamlibrarieswidget.h"
#include "registeredgameswidget.h"
#include "yamlconfigswidget.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QFrame>

GameTabWidget::GameTabWidget(QWidget *parent)
    : QWidget(parent)
{
    createUI();
}

GameTabWidget::~GameTabWidget()
{
}

void GameTabWidget::createUI()
{
    auto layout = new QVBoxLayout(this);
    
    tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("gameTabWidget");
    
    // Games section
    auto gamesSection = new QWidget(this);
    gamesSection->setObjectName("gamesSection");
    
    auto gamesSectionLayout = new QVBoxLayout(gamesSection);
    gamesSectionLayout->setObjectName("gamesSectionLayout");
    
    // Steam libraries widget
    steamLibrariesWidget = new SteamLibrariesWidget(this);
    steamLibrariesWidget->setObjectName("steamLibrariesWidget");
    gamesSectionLayout->addWidget(steamLibrariesWidget);
    
    // Horizontal divider
    auto divider = new QFrame(this);
    divider->setObjectName("divider");
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Sunken);
    gamesSectionLayout->addWidget(divider);
    
    // Registered games widget
    registeredGamesWidget = new RegisteredGamesWidget(this);
    registeredGamesWidget->setObjectName("registeredGamesWidget");
    gamesSectionLayout->addWidget(registeredGamesWidget);
    
    gamesSection->setLayout(gamesSectionLayout);
    tabWidget->addTab(gamesSection, "Games");
    
    // YAML Configurations section
    yamlConfigsWidget = new YAMLConfigsWidget(this);
    yamlConfigsWidget->setObjectName("yamlConfigsWidget");
    tabWidget->addTab(yamlConfigsWidget, "YAML Configurations");
    
    layout->addWidget(tabWidget);
    layout->setContentsMargins(0, 0, 0, 0);
}
