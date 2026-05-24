#ifndef REGISTEREDGAMESWIDGET_H
#define REGISTEREDGAMESWIDGET_H

#include <QWidget>

class QPushButton;
class QListWidget;

enum GameStatus
{
    Registered,
    ModCached,
    Validated
};

struct GameInfo
{
    int id;
    QString name;
    GameStatus status;
};

enum GameRole
{
    GameIdRole = Qt::UserRole + 1
};

class RegisteredGamesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RegisteredGamesWidget(QWidget *parent = nullptr);
    ~RegisteredGamesWidget() override;

public slots:
    void addGame(const GameInfo &gameInfo);
    void removeGame(int gameId);
    void updateGameStatus(int gameId, GameStatus status);
    void clearGames();

signals:
    void addGameClicked();
    void reloadGameRequested(int gameId);
    void deleteGameRequested(int gameId);
    void downloadModRequested(int gameId);

private:
    void createUI();
    void updateGameItem(int gameId, const GameInfo &gameInfo);

    QPushButton *addGameButton;
    QListWidget *gamesList;
};

#endif // REGISTEREDGAMESWIDGET_H
