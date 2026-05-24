#ifndef YAMLCONFIGSWIDGET_H
#define YAMLCONFIGSWIDGET_H

#include <QWidget>

class QPushButton;
class QListWidget;

enum class ConfigStatus
{
    Unvalidated,
    Validated
};

struct YAMLConfigInfo
{
    int id;
    QString name;
    QString gameName;
    ConfigStatus status;
};

enum class ConfigRole
{
    ConfigIdRole = Qt::UserRole + 1
};

class YAMLConfigsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit YAMLConfigsWidget(QWidget *parent = nullptr);
    ~YAMLConfigsWidget() override;

public slots:
    void addConfig(const YAMLConfigInfo &configInfo);
    void removeConfig(int configId);
    void updateConfigStatus(int configId, ConfigStatus status);
    void clearConfigs();

signals:
    void createNewConfigClicked();
    void editConfigRequested(int configId);
    void verifyConfigRequested(int configId);
    void quickLaunchRequested(int configId);
    void deleteConfigRequested(int configId);

private:
    void createUI();
    void updateConfigItem(int configId, const YAMLConfigInfo &configInfo);

    QPushButton *createNewConfigButton;
    QListWidget *configsList;
};

#endif // YAMLCONFIGSWIDGET_H
