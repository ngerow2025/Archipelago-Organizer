#pragma once
#include <qfileinfo.h>

#include <QDropEvent>
#include <QObject>
#include <QWidget>

#include "libzippp.h"

class APWorldWidget : public QWidget {
    Q_OBJECT

   public:
    explicit APWorldWidget(QWidget* parent = nullptr);

   protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;

   private:
    void installAPWorldFromDisk(const QString& filePath);
    void installAPWorldFromData(const void* data, uint64_t size, const QString& name);
    void installApWorld(const libzippp::ZipArchive* archive, const QString& name);
};
