//
// Created by birb on 9/29/26.
//

#include "APWorldSourceCard.h"

#include <QVBoxLayout>

#include "../LoadingSpinner.h"

APWorldSourceCard::APWorldSourceCard(QWidget *parent, APWorldGithubRemote* remote)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setObjectName("APWorldSourceCardLayout");

    auto* spinner = new LoadingSpinner(this);
    spinner->start();
    layout->addWidget(spinner);

    setLayout(layout);
}