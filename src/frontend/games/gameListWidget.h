#ifndef GAMELISTWIDGET_H
#define GAMELISTWIDGET_H

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

class GameListWidget : public QWidget {
    Q_OBJECT

   public:
    explicit GameListWidget(QWidget* parent = nullptr);

   private:
    QVBoxLayout* m_layout;
    QLabel*      m_emptyLabel;
};

#endif  // GAMELISTWIDGET_H
