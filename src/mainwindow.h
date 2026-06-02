#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>

class GameTabWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void createUI();

   private:
    QTabWidget*    tabWidget;
    GameTabWidget* gamesTab;
};
#endif  // MAINWINDOW_H
