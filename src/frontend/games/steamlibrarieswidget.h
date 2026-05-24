#ifndef STEAMLIBRARIESWIDGET_H
#define STEAMLIBRARIESWIDGET_H

#include <QWidget>

class QPushButton;
class QListWidget;

enum LibraryRole
{
    LibraryPathRole = Qt::UserRole + 1
};

class SteamLibrariesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SteamLibrariesWidget(QWidget *parent = nullptr);
    ~SteamLibrariesWidget() override;

public slots:
    void addLibrary(const QString &path);
    void removeLibrary(const QString &path);
    void clearLibraries();

signals:
    void scanForGamesClicked();
    void addFolderClicked();
    void deleteLibraryRequested(const QString &path);

private:
    void createUI();

    QPushButton *scanForGamesButton;
    QPushButton *addFolderButton;
    QListWidget *librariesList;
};

#endif // STEAMLIBRARIESWIDGET_H
