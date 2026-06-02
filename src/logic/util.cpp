#include "util.h"

#include <QDir>
#include <QFileInfo>

void printFileState(const QString& label, const QString& path) {
    const QFileInfo info(path);
    qDebug() << label << ":" << path << "\n"
             << "exists=" << info.exists() << "\n"
             << "isFile=" << info.isFile() << "\n"
             << "isExecutable=" << info.isExecutable() << "\n"
             << "isSymLink=" << info.isSymLink() << "\n"
             << "canonicalPath=" << info.canonicalFilePath() << "\n"
             << "symLinkTarget=" << info.symLinkTarget() << "\n";
}

bool clearDirectoryIfExists(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) {
        return true;
    }

    qDebug() << "Clearing directory:" << path;
    if (!dir.removeRecursively()) {
        qWarning() << "Failed to clear directory:" << path << "- Error:" << strerror(errno);
        return false;
    }

    return true;
}
