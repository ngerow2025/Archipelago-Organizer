#include "yamlconfigswidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

YAMLConfigsWidget::YAMLConfigsWidget(QWidget* parent) : QWidget(parent) {
    createUI();
}

YAMLConfigsWidget::~YAMLConfigsWidget() {
}

void YAMLConfigsWidget::createUI() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName("yamlConfigsLayout");

    // Header with title and button
    auto headerLayout = new QHBoxLayout();
    headerLayout->setObjectName("yamlConfigsHeaderLayout");

    auto titleLabel = new QLabel("YAML Configs");
    titleLabel->setObjectName("yamlConfigsTitle");
    headerLayout->addWidget(titleLabel);

    headerLayout->addStretch();

    createNewConfigButton = new QPushButton("Create new config");
    createNewConfigButton->setObjectName("createNewConfigButton");
    connect(createNewConfigButton, &QPushButton::clicked, this,
            &YAMLConfigsWidget::createNewConfigClicked);
    headerLayout->addWidget(createNewConfigButton);

    mainLayout->addLayout(headerLayout);

    // Configs list
    configsList = new QListWidget(this);
    configsList->setObjectName("configsList");
    mainLayout->addWidget(configsList);

    setLayout(mainLayout);
}

void YAMLConfigsWidget::addConfig(const YAMLConfigInfo& configInfo) {
    auto item = new QListWidgetItem(configsList);
    item->setData(static_cast<int>(ConfigRole::ConfigIdRole), configInfo.id);

    auto itemWidget = new QWidget();
    auto itemLayout = new QHBoxLayout(itemWidget);
    itemLayout->setContentsMargins(0, 0, 0, 0);

    // Config name
    auto nameLabel = new QLabel(configInfo.name);
    nameLabel->setObjectName("configNameLabel_" + QString::number(configInfo.id));
    itemLayout->addWidget(nameLabel);

    // Game name
    auto gameNameLabel = new QLabel(configInfo.gameName);
    gameNameLabel->setObjectName("configGameNameLabel_" + QString::number(configInfo.id));
    itemLayout->addWidget(gameNameLabel);

    // Status label
    QString statusText;
    QString statusColor;

    switch (configInfo.status) {
        case ConfigStatus::Unvalidated:
            statusText = "unvalidated";
            statusColor = "red";
            break;
        case ConfigStatus::Validated:
            statusText = "validated";
            statusColor = "green";
            break;
    }

    auto statusLabel = new QLabel(statusText);
    statusLabel->setObjectName("configStatusLabel_" + QString::number(configInfo.id));
    statusLabel->setStyleSheet("QLabel { color: " + statusColor + "; }");
    itemLayout->addWidget(statusLabel);

    itemLayout->addStretch();

    // Edit button
    auto editButton = new QPushButton("Edit");
    editButton->setObjectName("editConfigButton_" + QString::number(configInfo.id));
    connect(editButton, &QPushButton::clicked, this,
            [this, id = configInfo.id]() { emit editConfigRequested(id); });
    itemLayout->addWidget(editButton);

    // Verify button
    auto verifyButton = new QPushButton("Verify");
    verifyButton->setObjectName("verifyConfigButton_" + QString::number(configInfo.id));
    connect(verifyButton, &QPushButton::clicked, this,
            [this, id = configInfo.id]() { emit verifyConfigRequested(id); });
    itemLayout->addWidget(verifyButton);

    // Quick launch button
    auto quickLaunchButton = new QPushButton("Quick launch");
    quickLaunchButton->setObjectName("quickLaunchButton_" + QString::number(configInfo.id));
    connect(quickLaunchButton, &QPushButton::clicked, this,
            [this, id = configInfo.id]() { emit quickLaunchRequested(id); });
    itemLayout->addWidget(quickLaunchButton);

    // Delete button
    auto deleteButton = new QPushButton("Delete");
    deleteButton->setObjectName("deleteConfigButton_" + QString::number(configInfo.id));
    connect(deleteButton, &QPushButton::clicked, this,
            [this, id = configInfo.id]() { emit deleteConfigRequested(id); });
    itemLayout->addWidget(deleteButton);

    item->setSizeHint(itemWidget->sizeHint());
    configsList->setItemWidget(item, itemWidget);
}

void YAMLConfigsWidget::removeConfig(int configId) {
    for (int i = 0; i < configsList->count(); ++i) {
        auto item = configsList->item(i);
        if (item && item->data(static_cast<int>(ConfigRole::ConfigIdRole)).toInt() == configId) {
            delete configsList->takeItem(i);
            break;
        }
    }
}

void YAMLConfigsWidget::updateConfigStatus(int configId, ConfigStatus status) {
    for (int i = 0; i < configsList->count(); ++i) {
        auto item = configsList->item(i);
        if (item && item->data(static_cast<int>(ConfigRole::ConfigIdRole)).toInt() == configId) {
            auto widget = configsList->itemWidget(item);
            auto statusLabel =
                widget->findChild<QLabel*>("configStatusLabel_" + QString::number(configId));
            if (statusLabel) {
                QString statusText;
                QString statusColor;

                switch (status) {
                    case ConfigStatus::Unvalidated:
                        statusText = "unvalidated";
                        statusColor = "red";
                        break;
                    case ConfigStatus::Validated:
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

void YAMLConfigsWidget::clearConfigs() {
    configsList->clear();
}
