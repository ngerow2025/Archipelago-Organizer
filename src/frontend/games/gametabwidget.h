#ifndef GAMETABWIDGET_H
#define GAMETABWIDGET_H

#include <QWidget>

class QTabWidget;
class SteamLibrariesWidget;
class RegisteredGamesWidget;
class YAMLConfigsWidget;

class GameTabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameTabWidget(QWidget *parent = nullptr);
    ~GameTabWidget() override;

private:
    void createUI();

    QTabWidget *tabWidget;
    SteamLibrariesWidget *steamLibrariesWidget;
    RegisteredGamesWidget *registeredGamesWidget;
    YAMLConfigsWidget *yamlConfigsWidget;
};

#endif // GAMETABWIDGET_H
