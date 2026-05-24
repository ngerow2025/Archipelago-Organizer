#include "registeredgameswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>

RegisteredGamesWidget::RegisteredGamesWidget(QWidget *parent)
    : QWidget(parent)
{
    createUI();
}

RegisteredGamesWidget::~RegisteredGamesWidget()
{
}

void RegisteredGamesWidget::createUI()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName("registeredGamesLayout");
    
    // Header with title and button
    auto headerLayout = new QHBoxLayout();
    headerLayout->setObjectName("registeredGamesHeaderLayout");
    
    auto titleLabel = new QLabel("Registered Games");
    titleLabel->setObjectName("registeredGamesTitle");
    headerLayout->addWidget(titleLabel);
    
    headerLayout->addStretch();
    
    addGameButton = new QPushButton("Add game");
    addGameButton->setObjectName("addGameButton");
    connect(addGameButton, &QPushButton::clicked, this, &RegisteredGamesWidget::addGameClicked);
    headerLayout->addWidget(addGameButton);
    
    mainLayout->addLayout(headerLayout);
    
    // Games list
    gamesList = new QListWidget(this);
    gamesList->setObjectName("gamesList");
    mainLayout->addWidget(gamesList);
    
    setLayout(mainLayout);
}

void RegisteredGamesWidget::addGame(const GameInfo &gameInfo)
{
    auto item = new QListWidgetItem(gamesList);
    item->setData(GameIdRole, gameInfo.id);
    
    auto itemWidget = new QWidget();
    auto itemLayout = new QHBoxLayout(itemWidget);
    itemLayout->setContentsMargins(0, 0, 0, 0);
    
    // Game name
    auto nameLabel = new QLabel(gameInfo.name);
    nameLabel->setObjectName("gameNameLabel_" + QString::number(gameInfo.id));
    itemLayout->addWidget(nameLabel);
    
    // Status icon (placeholder label)
    QString statusText;
    QString statusColor;
    
    switch (gameInfo.status) {
        case Registered:
            statusText = "registered";
            statusColor = "red";
            break;
        case ModCached:
            statusText = "mod cached";
            statusColor = "orange";
            break;
        case Validated:
            statusText = "validated";
            statusColor = "green";
            break;
    }
    
    auto statusLabel = new QLabel(statusText);
    statusLabel->setObjectName("gameStatusLabel_" + QString::number(gameInfo.id));
    statusLabel->setStyleSheet("QLabel { color: " + statusColor + "; }");
    itemLayout->addWidget(statusLabel);
    
    itemLayout->addStretch();
    
    // Download mod button
    auto downloadButton = new QPushButton("Download mod");
    downloadButton->setObjectName("downloadModButton_" + QString::number(gameInfo.id));
    connect(downloadButton, &QPushButton::clicked, this, [this, id = gameInfo.id]() {
        emit downloadModRequested(id);
    });
    itemLayout->addWidget(downloadButton);
    
    // Reload button
    auto reloadButton = new QPushButton("Reload");
    reloadButton->setObjectName("reloadGameButton_" + QString::number(gameInfo.id));
    connect(reloadButton, &QPushButton::clicked, this, [this, id = gameInfo.id]() {
        emit reloadGameRequested(id);
    });
    itemLayout->addWidget(reloadButton);
    
    // Delete button
    auto deleteButton = new QPushButton("Delete");
    deleteButton->setObjectName("deleteGameButton_" + QString::number(gameInfo.id));
    connect(deleteButton, &QPushButton::clicked, this, [this, id = gameInfo.id]() {
        emit deleteGameRequested(id);
    });
    itemLayout->addWidget(deleteButton);
    
    item->setSizeHint(itemWidget->sizeHint());
    gamesList->setItemWidget(item, itemWidget);
}

void RegisteredGamesWidget::removeGame(int gameId)
{
    for (int i = 0; i < gamesList->count(); ++i) {
        auto item = gamesList->item(i);
        if (item && item->data(GameIdRole).toInt() == gameId) {
            delete gamesList->takeItem(i);
            break;
        }
    }
}

void RegisteredGamesWidget::updateGameStatus(int gameId, GameStatus status)
{
    for (int i = 0; i < gamesList->count(); ++i) {
        auto item = gamesList->item(i);
        if (item && item->data(GameIdRole).toInt() == gameId) {
            auto widget = gamesList->itemWidget(item);
            auto statusLabel = widget->findChild<QLabel *>("gameStatusLabel_" + QString::number(gameId));
            if (statusLabel) {
                QString statusText;
                QString statusColor;
                
                switch (status) {
                    case Registered:
                        statusText = "registered";
                        statusColor = "red";
                        break;
                    case ModCached:
                        statusText = "mod cached";
                        statusColor = "orange";
                        break;
                    case Validated:
                        statusText = "validated";
                        statusColor = "green";
                        break;
                }
                
                statusLabel->setText(statusText);
                statusLabel->setStyleSheet("QLabel { color: " + statusColor + "; }");
            }
            break;
        }
    }
}

void RegisteredGamesWidget::clearGames()
{
    gamesList->clear();
}
