//
// Created by birb on 9/29/26.
//

#ifndef ARCHIPELAGO_ORGANISER_APWORLDSOURCECARD_H
#define ARCHIPELAGO_ORGANISER_APWORLDSOURCECARD_H
#include <QWidget>

#include "../../logic/APWorldGithubRemote.h"

class APWorldSourceCard : public QWidget {
    Q_OBJECT

public:
    explicit APWorldSourceCard(QWidget *parent = nullptr, APWorldGithubRemote* remote = nullptr);
private:
    APWorldGithubRemote* m_remote;
};

#endif  // ARCHIPELAGO_ORGANISER_APWORLDSOURCECARD_H
