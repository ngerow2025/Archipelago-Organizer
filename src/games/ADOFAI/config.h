#ifndef ADOFAI_CONFIG_H
#define ADOFAI_CONFIG_H

#include <QString>

bool writeADOFAIConfig(const QString& configPath, const QString& pseudo, const QString& ip,
                       int port);

#endif  // ADOFAI_CONFIG_H