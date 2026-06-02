#ifndef UTIL_H
#define UTIL_H

#include <QString>

void printFileState(const QString& label, const QString& path);
bool clearDirectoryIfExists(const QString& path);

#endif  // UTIL_H