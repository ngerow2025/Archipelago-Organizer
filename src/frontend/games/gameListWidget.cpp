#include "gameListWidget.h"
#include <QVBoxLayout>
#include <QLabel>

GameListWidget::GameListWidget(QWidget *parent)
    : QWidget(parent) {
    setObjectName("gameListWidget");
    m_layout = new QVBoxLayout(this);
    m_layout->setObjectName("gameListLayout");

    m_emptyLabel = new QLabel("No games available", this);
    m_emptyLabel->setObjectName("emptyLabel");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_emptyLabel);

    setLayout(m_layout);
}
