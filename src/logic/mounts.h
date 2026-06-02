#ifndef MOUNTS_H
#define MOUNTS_H

#include <qstring.h>

bool mountOverlayFilesystem(const QString& mountPoint, const QStringList& lowerDirs,
                            const QString& upperDir, const QString& workDir);

bool mountBindFilesystem(const QString& sourcePath, const QString& targetPath);

#endif  // MOUNTS_H