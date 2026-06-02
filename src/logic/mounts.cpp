#include "mounts.h"

#include <qdebug.h>
#include <sys/mount.h>

#include <QList>

bool mountOverlayFilesystem(const QString& mountPoint, const QStringList& lowerDirs,
                            const QString& upperDir, const QString& workDir) {
    const QString opts = QString("lowerdir=%1,upperdir=%2,workdir=%3")
                             .arg(lowerDirs.join(':'))
                             .arg(upperDir)
                             .arg(workDir);

    qDebug() << "Mounting overlay filesystem at" << mountPoint << "with options:" << opts;
    if (mount("overlay", mountPoint.toStdString().c_str(), "overlay", 0,
              opts.toStdString().c_str()) != 0) {
        qWarning() << "Failed to mount overlay filesystem at" << mountPoint << ":"
                   << strerror(errno);
        return false;
    }

    return true;
}

bool mountBindFilesystem(const QString& sourcePath, const QString& targetPath) {
    qDebug() << "Bind mounting" << sourcePath << "to" << targetPath;
    if (mount(sourcePath.toStdString().c_str(), targetPath.toStdString().c_str(), nullptr, MS_BIND,
              nullptr) != 0) {
        qWarning() << "Failed to bind mount" << sourcePath << "to" << targetPath << ":"
                   << strerror(errno);
        return false;
    }

    return true;
}