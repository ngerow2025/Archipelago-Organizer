#include "steamlibrarieswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>

SteamLibrariesWidget::SteamLibrariesWidget(QWidget *parent)
    : QWidget(parent)
{
    createUI();
}

SteamLibrariesWidget::~SteamLibrariesWidget()
{
}

void SteamLibrariesWidget::createUI()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName("steamLibrariesLayout");
    
    // Header with title
    auto headerLayout = new QHBoxLayout();
    headerLayout->setObjectName("steamLibrariesHeaderLayout");
    
    auto titleLabel = new QLabel("Steam libraries");
    titleLabel->setObjectName("steamLibrariesTitle");
    headerLayout->addWidget(titleLabel);
    
    headerLayout->addStretch();
    
    scanForGamesButton = new QPushButton("Scan for games");
    scanForGamesButton->setObjectName("scanForGamesButton");
    connect(scanForGamesButton, &QPushButton::clicked, this, &SteamLibrariesWidget::scanForGamesClicked);
    headerLayout->addWidget(scanForGamesButton);
    
    addFolderButton = new QPushButton("Add folder");
    addFolderButton->setObjectName("addFolderButton");
    connect(addFolderButton, &QPushButton::clicked, this, &SteamLibrariesWidget::addFolderClicked);
    headerLayout->addWidget(addFolderButton);
    
    mainLayout->addLayout(headerLayout);
    
    // Libraries list
    librariesList = new QListWidget(this);
    librariesList->setObjectName("librariesList");
    mainLayout->addWidget(librariesList);
    
    setLayout(mainLayout);
}

void SteamLibrariesWidget::addLibrary(const QString &path)
{
    auto item = new QListWidgetItem(librariesList);
    item->setData(LibraryPathRole, path);
    
    auto itemWidget = new QWidget();
    auto itemLayout = new QHBoxLayout(itemWidget);
    itemLayout->setContentsMargins(0, 0, 0, 0);
    
    auto pathLabel = new QLabel(path);
    pathLabel->setObjectName("libraryPathLabel");
    itemLayout->addWidget(pathLabel);
    
    itemLayout->addStretch();
    
    auto deleteButton = new QPushButton("Delete");
    deleteButton->setObjectName("deleteLibraryButton_" + path);
    connect(deleteButton, &QPushButton::clicked, this, [this, path]() {
        emit deleteLibraryRequested(path);
    });
    itemLayout->addWidget(deleteButton);
    
    item->setSizeHint(itemWidget->sizeHint());
    librariesList->setItemWidget(item, itemWidget);
}

void SteamLibrariesWidget::removeLibrary(const QString &path)
{
    for (int i = 0; i < librariesList->count(); ++i) {
        auto item = librariesList->item(i);
        if (item && item->data(LibraryPathRole).toString() == path) {
            delete librariesList->takeItem(i);
            break;
        }
    }
}

void SteamLibrariesWidget::clearLibraries()
{
    librariesList->clear();
}
