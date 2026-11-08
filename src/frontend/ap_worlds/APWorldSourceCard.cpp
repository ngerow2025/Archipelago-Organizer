//
// Created by birb on 9/29/26.
//

#include "APWorldSourceCard.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>

#include "../LoadingSpinner.h"

APWorldSourceCard::APWorldSourceCard(QWidget *parent, APWorldGithubRemote* remote)
    : QWidget(parent), m_remote(remote)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setObjectName("APWorldSourceCardLayout");

    auto* spinner = new LoadingSpinner(this);
    spinner->start();
    layout->addWidget(spinner);

    setLayout(layout);

    connect(remote, &APWorldGithubRemote::allReleasesProcessed, this, [this, spinner]() {
        spinner->stop();
        spinner->deleteLater();
        //set the UI for the detected releases
        auto *listWidget = new QListWidget(this);
        listWidget->setObjectName("APWorldSourceCardReleaseList");
        auto releases = this->m_remote->getSortedReleases().value();
        for (const auto& [majorVersion, minorVersions] : releases) {
            for (const auto& [minorVersion, patchVersions] : minorVersions) {
                for (const auto& [patchVersion, releases] : patchVersions) {
                    for (const auto& release : releases) {
                        listWidget->addItem(release.name);
                    }
                }
            }
        }
        this->layout()->addWidget(listWidget);
    });
    remote->getRepoInfo();
}