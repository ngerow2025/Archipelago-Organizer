#include "config.h"

#include <qdebug.h>
#include <qsavefile.h>

#include <QFile>
#include <QRegularExpression>
#include <QtLogging>

bool writeADOFAIConfig(const QString& configPath, const QString& pseudo, const QString& ip,
                       int port) {
    QFile configFile(configPath);
    if (!configFile.exists()) {
        qWarning() << "Config file does not exist:" << configPath;
        return false;
    }

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open config file for reading:" << configPath
                   << "- Error:" << configFile.errorString();
        return false;
    }

    const QString originalContent = QString::fromUtf8(configFile.readAll());
    configFile.close();

    QString updatedContent = originalContent;
    updatedContent.replace(
        QRegularExpression(QStringLiteral("^pseudo\\s*=.*$"), QRegularExpression::MultilineOption),
        QStringLiteral("pseudo = %1").arg(pseudo));
    updatedContent.replace(
        QRegularExpression(QStringLiteral("^IP\\s*=.*$"), QRegularExpression::MultilineOption),
        QStringLiteral("IP = %1").arg(ip));
    updatedContent.replace(
        QRegularExpression(QStringLiteral("^Port\\s*=.*$"), QRegularExpression::MultilineOption),
        QStringLiteral("Port = %1").arg(port));

    if (updatedContent == originalContent) {
        qWarning() << "No config values were updated in:" << configPath;
        return false;
    }

    QSaveFile outputFile(configPath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open config file for writing:" << configPath
                   << "- Error:" << outputFile.errorString();
        return false;
    }

    QTextStream stream(&outputFile);
    stream << updatedContent;

    if (!outputFile.commit()) {
        qWarning() << "Failed to commit config file:" << configPath
                   << "- Error:" << outputFile.errorString();
        return false;
    }

    qDebug() << "Updated ADOFAI config:" << configPath;
    return true;
}