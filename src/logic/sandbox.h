#ifndef SANDBOX_H
#define SANDBOX_H

#include <qstring.h>

#include <functional>

struct SandboxResult {
    bool    success;
    int     exitCode;
    QString errorMessage;
};

SandboxResult executeInPidSandbox(const std::function<int()>& lambdaTask);

int activateUserNamespaceSandbox();

#endif  // SANDBOX_H