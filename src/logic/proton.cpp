#include "proton.h"

#include <unistd.h>

#include <QDebug>

#include "util.h"

void applyProtonEnvironment(const ProtonLaunchPaths& paths) {
    for (auto it = kProtonEnvVars.cbegin(); it != kProtonEnvVars.cend(); ++it) {
        qputenv(it.key().toUtf8(), it.value()(paths));
    }
}

void printProtonLaunchInfo(const ProtonLaunchPaths& paths) {
    qDebug() << "Proton launch paths:";
    qDebug() << "  compatBasePath:" << paths.compatBasePath;
    qDebug() << "  steamInstallPath:" << paths.steamInstallPath;
    qDebug() << "  protonPath:" << paths.protonPath;
    qDebug() << "  gameExePath:" << paths.gameExePath;

    qDebug() << "Proton environment variables:";
    for (auto it = kProtonEnvVars.cbegin(); it != kProtonEnvVars.cend(); ++it) {
        qDebug() << "  " << it.key() << '=' << it.value()(paths);
    }
}

int runProtonCommandAndWait(const ProtonLaunchPaths& paths, const QStringList& protonArgs) {
    printProtonLaunchInfo(paths);
    printFileState("Proton binary", paths.protonPath);
    applyProtonEnvironment(paths);

    QList<QByteArray> argStorage;
    argStorage.reserve(protonArgs.size() + 1);
    argStorage.append(paths.protonPath.toUtf8());
    for (const QString& arg : protonArgs) {
        argStorage.append(arg.toUtf8());
    }

    QList<char*> argv;
    argv.reserve(argStorage.size() + 1);
    for (QByteArray& arg : argStorage) {
        argv.append(arg.data());
    }
    argv.append(nullptr);

    execv(argStorage.first().constData(), argv.data());

    qWarning() << "Failed to exec Proton command:" << protonArgs << "- Error:" << strerror(errno);
    _exit(127);
}