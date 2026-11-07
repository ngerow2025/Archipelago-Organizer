//
// Created by birb on 7/6/26.
//

#include "APWorldWidget.h"

#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "../logic/LoadAPWorld.h"
#include "APWorldDragAndDrop.h"
#include "APWorldSourceCard.h"

APWorldWidget::APWorldWidget(QWidget* parent) : QWidget(parent) {
    setAcceptDrops(true);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setObjectName("APWorldWidgetLayout");

    QHBoxLayout* githubSourceLayout = new QHBoxLayout();

    QLineEdit* githubSourceRepo = new QLineEdit(this);
    githubSourceRepo->setPlaceholderText("Enter GitHub Repo URL");
    githubSourceLayout->addWidget(githubSourceRepo);

    QPushButton* addGithubSourceButton = new QPushButton("Add GitHub Source", this);
    githubSourceLayout->addWidget(addGithubSourceButton);

    QVBoxLayout* githubSources = new QVBoxLayout(this);
    githubSources->setObjectName("APWorldWidgetGithubSources");


    layout->addLayout(githubSourceLayout);
    layout->addLayout(githubSources);
    layout->addStretch();

    connect(addGithubSourceButton, &QPushButton::clicked, this, [this, githubSourceRepo, githubSources]() {
        QUrl repoUrl(githubSourceRepo->text());
        // check that it is a valid github url
        if (!repoUrl.isValid() || repoUrl.host() != "github.com") {
            qDebug() << "Invalid GitHub URL:" << repoUrl.toString();
            return;
        }
        // extract the repo name and owner from the url
        QStringList pathSegments = repoUrl.path().split('/', Qt::SkipEmptyParts);
        if (pathSegments.size() >= 2) {
            QString             owner = pathSegments[0];
            QString             repo = pathSegments[1];
            auto* remote = new APWorldGithubRemote(this, owner, repo);
            githubSources->addWidget(new APWorldSourceCard(this, remote));
        }
    });

    setLayout(layout);
}

void APWorldWidget::dropEvent(QDropEvent* event) {
    handleDropEvent(event);
}

void APWorldWidget::dragEnterEvent(QDragEnterEvent* event) {
    handleDragEnterEvent(event);
}
