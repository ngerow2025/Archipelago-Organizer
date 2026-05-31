#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QDir>
#include <QDebug>
#include <QDirIterator>

#include "sys/mount.h"
#include "sys/wait.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <iostream>

#include <csignal>
#include <cerrno>

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFile>
#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QMap>
#include <QSaveFile>
#include <QRegularExpression>
#include <QTextStream>

#include <functional>

#include <sys/capability.h>

#include <linux/prctl.h>
#include <sys/prctl.h>

#include <QCoreApplication>
#include <QString>
#include <QDebug>

#include <functional>
#include <atomic>
#include <chrono>
#include <vector>
#include <algorithm>
#include <sched.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/mount.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>

// Global variable to hold the child's process group ID
volatile sig_atomic_t game_pgid = 0;

// Track the namespace init process from the parent side
volatile sig_atomic_t ns_init_pid = 0;

// Track if we've already sent a termination signal
volatile sig_atomic_t signal_sent = 0;

struct ProtonLaunchPaths
{
    QString compatBasePath;
    QString steamInstallPath;
    QString protonPath;
    QString gameExePath;
    QString compatInstallPath;
};

static const QMap<QString, std::function<QByteArray(const ProtonLaunchPaths &)>> kProtonEnvVars = {
    {QStringLiteral("STEAM_COMPAT_DATA_PATH"), [](const ProtonLaunchPaths &paths)
     { return paths.compatBasePath.toUtf8(); }},
    {QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"), [](const ProtonLaunchPaths &paths)
     { return paths.steamInstallPath.toUtf8(); }},
    {QStringLiteral("STEAM_COMPAT_INSTALL_PATH"), [](const ProtonLaunchPaths &paths)
     { return paths.compatInstallPath.toUtf8(); }},
    {QStringLiteral("STEAM_COMPAT_LIBRARY_PATHS"), [](const ProtonLaunchPaths &paths)
     { return paths.compatInstallPath.toUtf8(); }},
    {QStringLiteral("PROTON_LOG"), [](const ProtonLaunchPaths &)
     { return QByteArrayLiteral("1"); }},
    {QStringLiteral("WINEDLLOVERRIDES"), [](const ProtonLaunchPaths &)
     { return QByteArrayLiteral("winhttp=n,b"); }},
    {QStringLiteral("WINEDEBUG"), [](const ProtonLaunchPaths &)
     { return QByteArrayLiteral("-all"); }},
};

static void applyProtonEnvironment(const ProtonLaunchPaths &paths)
{
    for (auto it = kProtonEnvVars.cbegin(); it != kProtonEnvVars.cend(); ++it)
    {
        qputenv(it.key().toUtf8(), it.value()(paths));
    }
}

static void printProtonLaunchInfo(const ProtonLaunchPaths &paths)
{
    qDebug() << "Proton launch paths:";
    qDebug() << "  compatBasePath:" << paths.compatBasePath;
    qDebug() << "  steamInstallPath:" << paths.steamInstallPath;
    qDebug() << "  protonPath:" << paths.protonPath;
    qDebug() << "  gameExePath:" << paths.gameExePath;
    qDebug() << "  compatInstallPath:" << paths.compatInstallPath;

    qDebug() << "Proton environment variables:";
    for (auto it = kProtonEnvVars.cbegin(); it != kProtonEnvVars.cend(); ++it)
    {
        qDebug() << "  " << it.key() << '=' << it.value()(paths);
    }
}

static void printFileState(const QString &label, const QString &path)
{
    const QFileInfo info(path);
    qDebug() << label << ":" << path
             << "exists=" << info.exists()
             << "isFile=" << info.isFile()
             << "isExecutable=" << info.isExecutable()
             << "isSymLink=" << info.isSymLink()
             << "canonicalPath=" << info.canonicalFilePath()
             << "symLinkTarget=" << info.symLinkTarget();
}

static bool clearDirectoryIfExists(const QString &path)
{
    QDir dir(path);
    if (!dir.exists())
    {
        return true;
    }

    qDebug() << "Clearing directory:" << path;
    if (!dir.removeRecursively())
    {
        qWarning() << "Failed to clear directory:" << path << "- Error:" << strerror(errno);
        return false;
    }

    return true;
}

static bool mountOverlayFilesystem(const QString &mountPoint, const QStringList &lowerDirs, const QString &upperDir, const QString &workDir)
{
    const QString opts = QString("lowerdir=%1,upperdir=%2,workdir=%3")
                             .arg(lowerDirs.join(':'))
                             .arg(upperDir)
                             .arg(workDir);

    qDebug() << "Mounting overlay filesystem at" << mountPoint << "with options:" << opts;
    if (mount("overlay", mountPoint.toStdString().c_str(), "overlay",
              0, opts.toStdString().c_str()) != 0)
    {
        qWarning() << "Failed to mount overlay filesystem at" << mountPoint << ":" << strerror(errno);
        return false;
    }

    return true;
}

static bool mountBindFilesystem(const QString &sourcePath, const QString &targetPath)
{
    qDebug() << "Bind mounting" << sourcePath << "to" << targetPath;
    if (mount(sourcePath.toStdString().c_str(), targetPath.toStdString().c_str(), nullptr,
              MS_BIND, nullptr) != 0)
    {
        qWarning() << "Failed to bind mount" << sourcePath << "to" << targetPath << ":" << strerror(errno);
        return false;
    }

    return true;
}

static int runProtonCommandAndWait(const ProtonLaunchPaths &paths, const QStringList &protonArgs)
{
    pid_t commandPid = fork();
    if (commandPid == 0)
    {
        setsid();

        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);

        printProtonLaunchInfo(paths);
        printFileState("Proton binary", paths.protonPath);
        applyProtonEnvironment(paths);

        QList<QByteArray> argStorage;
        argStorage.reserve(protonArgs.size() + 1);
        argStorage.append(paths.protonPath.toUtf8());
        for (const QString &arg : protonArgs)
        {
            argStorage.append(arg.toUtf8());
        }

        QList<char *> argv;
        argv.reserve(argStorage.size() + 1);
        for (QByteArray &arg : argStorage)
        {
            argv.append(arg.data());
        }
        argv.append(nullptr);

        execv(argStorage.first().constData(), argv.data());

        qWarning() << "Failed to exec Proton command:" << protonArgs << "- Error:" << strerror(errno);
        _exit(127);
    }

    if (commandPid < 0)
    {
        qWarning() << "Failed to fork Proton command process:" << strerror(errno);
        return -1;
    }

    int status = 0;
    while (true)
    {
        pid_t waitPid = waitpid(commandPid, &status, 0);
        if (waitPid == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            qWarning() << "Failed while waiting for Proton command:" << strerror(errno);
            return -1;
        }

        break;
    }

    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status))
    {
        return 128 + WTERMSIG(status);
    }

    return status;
}

static bool writeADOFAIConfig(const QString &configPath, const QString &pseudo, const QString &ip, int port)
{
    QFile configFile(configPath);
    if (!configFile.exists())
    {
        qWarning() << "Config file does not exist:" << configPath;
        return false;
    }

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open config file for reading:" << configPath << "- Error:" << configFile.errorString();
        return false;
    }

    const QString originalContent = QString::fromUtf8(configFile.readAll());
    configFile.close();

    QString updatedContent = originalContent;
    updatedContent.replace(QRegularExpression(QStringLiteral("^pseudo\\s*=.*$"), QRegularExpression::MultilineOption), QStringLiteral("pseudo = %1").arg(pseudo));
    updatedContent.replace(QRegularExpression(QStringLiteral("^IP\\s*=.*$"), QRegularExpression::MultilineOption), QStringLiteral("IP = %1").arg(ip));
    updatedContent.replace(QRegularExpression(QStringLiteral("^Port\\s*=.*$"), QRegularExpression::MultilineOption), QStringLiteral("Port = %1").arg(port));

    if (updatedContent == originalContent)
    {
        qWarning() << "No config values were updated in:" << configPath;
        return false;
    }

    QSaveFile outputFile(configPath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open config file for writing:" << configPath << "- Error:" << outputFile.errorString();
        return false;
    }

    QTextStream stream(&outputFile);
    stream << updatedContent;

    if (!outputFile.commit())
    {
        qWarning() << "Failed to commit config file:" << configPath << "- Error:" << outputFile.errorString();
        return false;
    }

    qDebug() << "Updated ADOFAI config:" << configPath;
    return true;
}

static QString argumentValue(int argc, char *argv[], const QString &name, const QString &fallback)
{
    const QString prefix = name + "=";
    for (int i = 1; i < argc; ++i)
    {
        const QString current = QString::fromLocal8Bit(argv[i]);
        if (current == name && i + 1 < argc)
        {
            return QString::fromLocal8Bit(argv[i + 1]);
        }
        if (current.startsWith(prefix))
        {
            return current.mid(prefix.size());
        }
    }

    return fallback;
}

// The function that runs when Ctrl+C is pressed inside the namespace init process
void handle_shutdown(int sig)
{
    if (game_pgid > 0)
    {
        if (signal_sent == 0)
        {
            // First signal: try graceful termination
            qDebug() << "\nSignal" << sig << "caught! Sending SIGTERM to process group" << game_pgid;
            kill(-game_pgid, SIGTERM);
            signal_sent = 1;
        }
        else if (signal_sent == 1)
        {
            // Second signal: force kill if process didn't respond to SIGTERM
            qDebug() << "\nSecond signal received. Process didn't respond to SIGTERM. Sending SIGKILL to process group" << game_pgid;
            kill(-game_pgid, SIGKILL);
            signal_sent = 2;
        }
        // Further signals are ignored - process should be dead by now
    }
}

// The function that runs when Ctrl+C is pressed in the parent process
void handle_namespace_init_signal(int sig)
{
    if (ns_init_pid > 0)
    {
        if (signal_sent == 0)
        {
            qDebug() << "\nSignal" << sig << "caught! Sending SIGTERM to namespace init process" << ns_init_pid;
            kill(ns_init_pid, SIGTERM);
            signal_sent = 1;
        }
        else if (signal_sent == 1)
        {
            qDebug() << "\nSecond signal received. Namespace init process did not respond to SIGTERM. Sending SIGKILL to namespace init process" << ns_init_pid;
            kill(ns_init_pid, SIGKILL);
            signal_sent = 2;
        }
    }
}

// Function to read and print a specific POSIX capability from the current process
void printCapability(const char *capName, cap_value_t cap)
{
    cap_t caps = cap_get_proc();
    if (caps == nullptr)
    {
        qWarning() << "[main][bootstrap] Failed to get capabilities:" << strerror(errno);
        return;
    }

    cap_flag_value_t capValue;
    if (cap_get_flag(caps, cap, CAP_EFFECTIVE, &capValue) != 0)
    {
        qWarning() << "[main][bootstrap] Failed to get" << capName << "capability:" << strerror(errno);
        cap_free(caps);
        return;
    }

    qDebug() << "[main][bootstrap]" << capName << ":" << ((capValue == CAP_SET) ? "Yes" : "No");
    cap_free(caps);
}

static QByteArray fileHash(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return {};

    QCryptographicHash hash(QCryptographicHash::Md5);
    if (!hash.addData(&f))
        return {};

    return hash.result();
}

static bool filesAreIdentical(const QString &pathA, const QString &pathB)
{
    QFileInfo a(pathA), b(pathB);

    if (a.size() != b.size())
        return false;

    const QByteArray hashA = fileHash(pathA);
    const QByteArray hashB = fileHash(pathB);

    return !hashA.isEmpty() && (hashA == hashB);
}

bool copyRecursively(const QString &sourcePath, const QString &destPath)
{
    QDir sourceDir(sourcePath);
    if (!sourceDir.exists())
    {
        qWarning() << "[FAILURE] Source directory does not exist:" << sourcePath << "- Error:" << strerror(errno);
        return false;
    }

    QDir destDir(destPath);
    if (!destDir.exists())
    {
        qDebug() << "[MKDIR] Creating destination directory:" << destPath;
        if (!destDir.mkpath("."))
        {
            qWarning() << "[FAILURE] Failed to create destination directory:" << destPath << "- Error:" << strerror(errno);
            return false;
        }
    }

    const QFileInfoList entries = sourceDir.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);

    for (const QFileInfo &entry : entries)
    {
        const QString srcEntry = entry.absoluteFilePath();
        const QString destEntry = destPath + QDir::separator() + entry.fileName();

        if (entry.isDir())
        {
            if (!copyRecursively(srcEntry, destEntry))
            {
                qWarning() << "[FAILURE] Recursive copy failed for directory:" << srcEntry;
                return false;
            }
        }
        else
        {
            if (QFile::exists(destEntry))
            {
                if (filesAreIdentical(srcEntry, destEntry))
                {
                    qDebug() << "[IDENTICAL]  Skipping:" << destEntry;
                    continue;
                }

                qDebug() << "[OVERWRITE]  Replacing:" << destEntry;
                if (!QFile::remove(destEntry))
                {
                    QFileInfo fileInfo(destEntry);
                    qWarning() << "[FAILURE] Failed to remove existing file:" << destEntry
                               << "- Writable:" << fileInfo.isWritable()
                               << "- Error:" << strerror(errno);
                    return false;
                }
            }
            qDebug() << "[COPY]  Copying:" << srcEntry << "to" << destEntry;
            if (!QFile::copy(srcEntry, destEntry))
            {
                QFileInfo srcInfo(srcEntry);
                qWarning() << "[FAILURE] Failed to copy file from" << srcEntry << "to" << destEntry
                           << "- Source readable:" << srcInfo.isReadable()
                           << "- Dest dir writable:" << QFileInfo(destPath).isWritable()
                           << "- Error:" << strerror(errno);
                return false;
            }
        }
    }

    return true;
}

std::vector<pid_t> getActivePidsInsideNamespace()
{
    std::vector<pid_t> pids;
    DIR *dir = opendir("/proc");
    if (!dir)
    {
        qWarning() << "[PID 1] Failed to open /proc to scan for alive processes.";
        return pids;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr)
    {
        if (ent->d_type == DT_DIR)
        {
            QString name = ent->d_name;
            bool ok;
            pid_t pid = name.toInt(&ok);
            // Ignore PID 1 (itself). Any other numeric folder is an active process.
            if (ok && pid > 1)
            {
                pids.push_back(pid);
            }
        }
    }
    closedir(dir);
    return pids;
}

static QString processNameForPid(pid_t pid)
{
    QFile commFile(QStringLiteral("/proc/%1/comm").arg(pid));
    if (!commFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QStringLiteral("<unknown>");
    }

    const QString name = QString::fromUtf8(commFile.readAll()).trimmed();
    return name.isEmpty() ? QStringLiteral("<unknown>") : name;
}

static void printSandboxProcessSnapshot(const QString &label)
{
    std::vector<pid_t> pids = getActivePidsInsideNamespace();
    std::sort(pids.begin(), pids.end());

    qDebug() << "[PID 1]" << label << "active sandbox processes:";
    if (pids.empty())
    {
        qDebug() << "[PID 1]   (none)";
        return;
    }

    qDebug() << "[PID 1]   PID  COMMAND";
    for (pid_t pid : pids)
    {
        qDebug().noquote() << QStringLiteral("[PID 1]   %1  %2")
                                  .arg(pid, 4)
                                  .arg(processNameForPid(pid));
    }
}

// --- GLOBALS FOR SIGNAL HANDLING ---
static pid_t g_workerPid = -1;
static std::atomic<int> g_pendingSignal{0};

void atomicSignalHandler(int signum)
{
    g_pendingSignal.store(signum);
}

// --- DATA STRUCTURES ---
struct SandboxResult
{
    bool success;
    int exitCode;
    QString errorMessage;
};

// --- THE SINGLE ENTRY POINT UTILITY ---
SandboxResult executeInPidSandbox(const std::function<int()> &lambdaTask)
{
    g_pendingSignal.store(0);
    pid_t parentPid = getpid();
    pid_t launcherPid = fork();

    if (launcherPid < 0)
        return {false, -1, QString("Fork failed.")};

    if (launcherPid == 0)
    {
        prctl(PR_SET_PDEATHSIG, SIGKILL);
        if (getppid() != parentPid)
            _exit(EXIT_FAILURE);

        if (unshare(CLONE_NEWPID | CLONE_NEWNS) != 0)
            _exit(EXIT_FAILURE);
        if (mount("none", "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0)
            _exit(EXIT_FAILURE);

        pid_t rootPid = fork();
        if (rootPid < 0)
            _exit(EXIT_FAILURE);

        if (rootPid == 0)
        {
            // ==========================================
            // FORK 2: THE NAMESPACE ROOT (PID 1)
            // ==========================================
            prctl(PR_SET_PDEATHSIG, SIGKILL);
            if (getppid() != 0)
                _exit(EXIT_FAILURE);
            if (mount("proc", "/proc", "proc", 0, NULL) != 0)
                _exit(EXIT_FAILURE);

            struct sigaction sa;
            sa.sa_handler = atomicSignalHandler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGTERM, &sa, nullptr);
            sigaction(SIGINT, &sa, nullptr);

            g_workerPid = fork();
            if (g_workerPid < 0)
                _exit(EXIT_FAILURE);

            if (g_workerPid == 0)
            {
                // ==========================================
                // FORK 3: THE WORKER PROCESS (PID 2)
                // ==========================================
                prctl(PR_SET_PDEATHSIG, SIGKILL);
                if (getppid() != 1)
                    _exit(EXIT_FAILURE);
                _exit(lambdaTask());
            }

            // ==========================================
            // PID 1: THE EVENT LOOP
            // ==========================================
            bool workerDeadGracePeriod = false;
            bool externalShutdownInitiated = false;
            auto gracePeriodStart = std::chrono::steady_clock::now();
            const auto GRACE_PERIOD = std::chrono::seconds(10);
            int finalExitStatus = EXIT_FAILURE;

            while (true)
            {
                // 1. Check for external shutdown signals
                int sig = g_pendingSignal.exchange(0);
                if (sig != 0 && !externalShutdownInitiated && !workerDeadGracePeriod)
                {
                    qDebug().noquote() << "[PID 1] External signal received. Broadcasting to sandbox...";
                    kill(-1, sig);
                    externalShutdownInitiated = true;
                    gracePeriodStart = std::chrono::steady_clock::now();
                }

                // 2. Reap Processes
                // Use WNOHANG if we are waiting for timers, otherwise block normally.
                int options = (workerDeadGracePeriod || externalShutdownInitiated) ? WNOHANG : 0;
                int status = 0;
                pid_t reapedPid = waitpid(-1, &status, options);

                if (reapedPid > 0)
                {
                    if (reapedPid == g_workerPid)
                    {
                        qDebug() << "[PID 1] Main worker exited. Initiating 10-second grace period for remaining background processes.";

                        // Save the worker's exit status so PID 1 can return it later
                        if (WIFEXITED(status))
                            finalExitStatus = WEXITSTATUS(status);
                        else if (WIFSIGNALED(status))
                            finalExitStatus = 128 + WTERMSIG(status);

                        workerDeadGracePeriod = true;
                        gracePeriodStart = std::chrono::steady_clock::now();

                        printSandboxProcessSnapshot(QStringLiteral("Process snapshot #1 before SIGINT"));
                        sleep(1);
                        printSandboxProcessSnapshot(QStringLiteral("Process snapshot #2 before SIGINT"));

                        // Broadcast SIGINT to give remaining processes a chance to clean up
                        kill(-1, SIGINT);
                    }
                    else
                    {
                        qDebug() << "[PID 1] Successfully reaped background process PID:" << reapedPid;
                    }
                }
                else if (reapedPid == 0)
                {
                    // Check our timeout
                    if (workerDeadGracePeriod || externalShutdownInitiated)
                    {
                        if (std::chrono::steady_clock::now() - gracePeriodStart > GRACE_PERIOD)
                        {
                            qWarning() << "------------------------------------------";
                            qWarning() << "[PID 1] 10-second grace period expired!";

                            std::vector<pid_t> alivePids = getActivePidsInsideNamespace();
                            for (pid_t p : alivePids)
                            {
                                qWarning() << "[PID 1] Process" << p << "is still alive! Sending targeted SIGKILL.";
                                kill(p, SIGKILL);
                            }
                            qWarning() << "------------------------------------------";

                            // Exit PID 1, letting the kernel act as a final fallback kill
                            _exit(finalExitStatus);
                        }
                        usleep(50000); // 50ms polling rest
                    }
                }
                else
                {
                    if (errno == EINTR)
                        continue;
                    if (errno == ECHILD)
                    {
                        qDebug() << "[PID 1] All sandbox processes have cleanly exited. Shutting down namespace.";
                        if (workerDeadGracePeriod)
                            _exit(finalExitStatus);
                        break;
                    }
                    _exit(EXIT_FAILURE);
                }
            }
            _exit(EXIT_FAILURE);
        }

        // ==========================================
        // BACK IN LAUNCHER PROCESS
        // ==========================================
        struct sigaction saLauncher;
        saLauncher.sa_handler = atomicSignalHandler;
        sigemptyset(&saLauncher.sa_mask);
        saLauncher.sa_flags = 0;
        sigaction(SIGTERM, &saLauncher, nullptr);
        sigaction(SIGINT, &saLauncher, nullptr);

        int rootStatus = 0;
        while (true)
        {
            int sig = g_pendingSignal.exchange(0);
            if (sig != 0)
                kill(rootPid, sig);

            if (waitpid(rootPid, &rootStatus, 0) == -1)
            {
                if (errno == EINTR)
                    continue;
                _exit(EXIT_FAILURE);
            }
            break;
        }

        if (WIFEXITED(rootStatus))
            _exit(WEXITSTATUS(rootStatus));
        _exit(128 + WTERMSIG(rootStatus));
    }

    // ==========================================
    // BACK IN MAIN QT PROCESS
    // ==========================================
    int launcherStatus = 0;
    while (waitpid(launcherPid, &launcherStatus, 0) == -1)
    {
        if (errno == EINTR)
            continue;
        return {false, -1, QString("waitpid failed: %1").arg(strerror(errno))};
    }

    if (WIFEXITED(launcherStatus))
        return {true, WEXITSTATUS(launcherStatus), QString()};
    return {false, -1, QString("Launcher process failed.")};
}

static std::atomic<bool> g_daemonReceivedSigInt{false};

int main(int argc, char *argv[])
{

    { // print out the current UID and GID to verify that we are now root inside the namespace
        qDebug() << "[main][bootstrap] Current UID:" << getuid() << "(should be 0)";
        qDebug() << "[main][bootstrap] Current GID:" << getgid() << "(should be 0)";

        // print out the current PID
        qDebug() << "[main][bootstrap] Current PID:" << getpid();

        // Print out relevant POSIX capabilities
        printCapability("CAP_SETUID", CAP_SETUID);
        printCapability("CAP_SETGID", CAP_SETGID);
        printCapability("CAP_SETFCAP", CAP_SETFCAP);

        // get calling UID and GID to set up the user namespace mapping
        uid_t uid = getuid();
        gid_t gid = getgid();

        // start by creating a user namespace so that this process executes with pseudo-root privileges, allowing us to mount the overlay filesystem without root.
        if (unshare(CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWIPC) != 0)
        {
            qWarning() << "Failed to unshare user namespace:" << strerror(errno);
            return -1;
        }
        qDebug() << "[main][bootstrap] User namespace unshared successfully.";

        // print out the current PID
        qDebug() << "[main][bootstrap] Current PID:" << getpid();

        // read current capabilities using capget to verify that we have the necessary capabilities to perform the mount
        // spesifically check CAP_SETUID, CAP_SETGID, CAP_SETFCAP
        printCapability("CAP_SETUID", CAP_SETUID);
        printCapability("CAP_SETGID", CAP_SETGID);
        printCapability("CAP_SETFCAP", CAP_SETFCAP);

        // edit the user mapping in /proc/self/uid_map to map the current user to root inside the namespace
        QString uidMap = QString("0 %1 1").arg(uid);
        QString gidMap = QString("0 %1 1").arg(gid);
        QFile uidMapFile("/proc/self/uid_map");
        if (!uidMapFile.open(QIODevice::WriteOnly))
        {
            qWarning() << "Failed to open uid_map for writing:" << strerror(errno);
            return -1;
        }
        if (uidMapFile.write(uidMap.toUtf8()) == -1)
        {
            qWarning() << "Failed to write to uid_map:" << strerror(errno);
            return -1;
        }
        uidMapFile.close();

        // write "deny" to /proc/self/setgroups to prevent the kernel from rejecting our GID mapping
        QFile setGroupsFile("/proc/self/setgroups");
        if (!setGroupsFile.open(QIODevice::WriteOnly))
        {
            qWarning() << "Failed to open setgroups for writing:" << strerror(errno);
            return -1;
        }
        if (setGroupsFile.write("deny") == -1)
        {
            qWarning() << "Failed to write to setgroups:" << strerror(errno);
            return -1;
        }
        setGroupsFile.close();

        QFile gidMapFile("/proc/self/gid_map");
        if (!gidMapFile.open(QIODevice::WriteOnly))
        {
            qWarning() << "Failed to open gid_map for writing:" << strerror(errno);
            return -1;
        }
        if (gidMapFile.write(gidMap.toUtf8()) == -1)
        {
            qWarning() << "Failed to write to gid_map:" << strerror(errno);
            return -1;
        }
        gidMapFile.close();

        // print out the current UID and GID to verify that we are now root inside the namespace
        qDebug() << "[main][bootstrap] Current UID:" << getuid() << "(should be 0)";
        qDebug() << "[main][bootstrap] Current GID:" << getgid() << "(should be 0)";
    }


    // QApplication a(argc, argv);
    // MainWindow w;
    // QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    // int targetWidth = screenGeometry.width() * 0.8;
    // int targetHeight = screenGeometry.height() * 0.8;
    // w.resize(targetWidth, targetHeight);
    // w.show();
    // return QApplication::exec();

    QLoggingCategory::setFilterRules("filecopy.info=true");

    QString steamDir = "/home/birb/.local/share/Steam/steamapps/common/";
    QString ADOFAIDir = "/home/birb/.local/share/Steam/steamapps/common/A Dance of Fire and Ice/";
    // find all the folders in the steam directory
    QDir steamDirObj(ADOFAIDir);
    if (!steamDirObj.exists())
    {
        qWarning() << "[main][environment] Steam directory does not exist:" << steamDir;
        return -1;
    }
    else
    {
        qDebug() << "[main][environment] Steam directory found:" << steamDir;
        QStringList folders = steamDirObj.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        qDebug() << "[main][environment] Found folders in Steam directory:" << folders;
    }

    QString gameDir = "./Game/";
    QString bepinexDir = "./BepInEx_win_x64_5.4.23.5/";
    QString modDir = "./mod/ADOFAI_AP-Mod/";
    QString upperDir = "./upper/";
    QString mergeDir = "./merge/";
    QString setupDriveDir = "./setup_a_drive/";
    QString workDir = "./work/";
    QString configPath = QDir(upperDir).filePath("BepInEx/config/com.shotal.ADOFAI_AP.cfg");

    const QString pseudo = argumentValue(argc, argv, "--pseudo", "Apple");
    const QString ip = argumentValue(argc, argv, "--ip", "localhost");
    const int port = argumentValue(argc, argv, "--port", "38281").toInt();

    // if (!writeADOFAIConfig(configPath, pseudo, ip, port))
    // {
    //     return -1;
    // }

    QString compatBaseDir = "./compat_base/";
    QString compatOverlayDir = "./compat_base_overlay/";
    QString compatOverlayUpperDir = "./compat_base_overlay_upper/";
    QString compatOverlayWorkDir = "./compat_base_overlay_work/";

    // ensure that work is empty
    QDir workDirObj(workDir);
    if (workDirObj.exists())
    {
        qDebug() << "[main][filesystem-prep] Cleaning work directory:" << workDir;
        workDirObj.removeRecursively();
    }
    workDirObj.mkpath(".");

    if (!clearDirectoryIfExists(compatBaseDir))
    {
        return -1;
    }

    if (!clearDirectoryIfExists(compatOverlayDir) ||
        !clearDirectoryIfExists(compatOverlayUpperDir) ||
        !clearDirectoryIfExists(compatOverlayWorkDir))
    {
        return -1;
    }

    if (!clearDirectoryIfExists(setupDriveDir))
    {
        return -1;
    }

    QDir().mkpath(compatBaseDir);
    QDir().mkpath(compatOverlayDir);
    QDir().mkpath(compatOverlayUpperDir);
    QDir().mkpath(compatOverlayWorkDir);
    QDir().mkpath(setupDriveDir);

    // ensure that each required dir exists
    QDir().mkpath(gameDir);
    QDir().mkpath(upperDir);
    QDir().mkpath(mergeDir);

    QDir bepinexDirObj(bepinexDir);
    if (!bepinexDirObj.exists())
    {
        qWarning() << "[main][filesystem-prep] BepInEx directory does not exist:" << bepinexDir;
        return -1;
    }

    QDir modDirObj(modDir);
    if (!modDirObj.exists())
    {
        qWarning() << "[main][filesystem-prep] Mod directory does not exist:" << modDir;
        return -1;
    }

    const SandboxResult setupSandbox = executeInPidSandbox([&]() -> int
    {
        ProtonLaunchPaths initPaths{
            .compatBasePath = QDir::cleanPath(QDir::current().absoluteFilePath(compatBaseDir)),
            .steamInstallPath = QDir::cleanPath(QDir::home().filePath(".steam/steam")),
            .protonPath = QDir::cleanPath(QDir::home().filePath(".steam/steam/steamapps/common/Proton 10.0/proton")),
            .gameExePath = QDir::cleanPath(QDir::current().absoluteFilePath("hello_world.exe")),
            .compatInstallPath = QDir::cleanPath(QDir::current().absoluteFilePath(mergeDir)),
        };

        const QString setupExecutableSource = QDir::cleanPath(QDir::current().absoluteFilePath("hello_world.exe"));
        const QString setupDriveMountPath = QDir::cleanPath(QDir::current().absoluteFilePath(setupDriveDir));
        const QString setupDriveExecutablePath = QDir(setupDriveMountPath).filePath("hello_world.exe");
        const QString setupDosdevicesPath = compatBaseDir + "pfx/dosdevices/";
        const QString setupDriveSymlinkPath = setupDosdevicesPath + "a:";

        const auto cleanupSetupDrive = [&]()
        {
            if (QFileInfo(setupDriveSymlinkPath).isSymLink() || QFile::exists(setupDriveSymlinkPath))
            {
                QFile::remove(setupDriveSymlinkPath);
            }

            if (umount(setupDriveExecutablePath.toStdString().c_str()) != 0)
            {
                qWarning() << "[main][setup-sandbox] Failed to unmount setup executable bind mount:" << strerror(errno);
            }

            if (!clearDirectoryIfExists(setupDriveDir))
            {
                qWarning() << "[main][setup-sandbox] Failed to clear setup drive directory:" << setupDriveDir;
            }
        };

        QDir setupDosdevicesDir(setupDosdevicesPath);
        if (!setupDosdevicesDir.exists())
        {
            qDebug() << "[main][setup-sandbox] Creating setup dosdevices directory:" << setupDosdevicesPath;
            if (!setupDosdevicesDir.mkpath("."))
            {
                qWarning() << "[main][setup-sandbox] Failed to create setup dosdevices directory:" << setupDosdevicesPath << "- Error:" << strerror(errno);
                return 1;
            }
        }

        QFile setupDriveExecutablePlaceholder(setupDriveExecutablePath);
        if (!setupDriveExecutablePlaceholder.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            qWarning() << "[main][setup-sandbox] Failed to create setup drive mount point:" << setupDriveExecutablePath << "- Error:" << setupDriveExecutablePlaceholder.errorString();
            return 1;
        }
        setupDriveExecutablePlaceholder.close();

        if (!mountBindFilesystem(setupExecutableSource, setupDriveExecutablePath))
        {
            return 1;
        }

        if (QFileInfo(setupDriveSymlinkPath).isSymLink() || QFile::exists(setupDriveSymlinkPath))
        {
            qDebug() << "[main][setup-sandbox] Removing existing setup symlink/file:" << setupDriveSymlinkPath;
            if (!QFile::remove(setupDriveSymlinkPath))
            {
                qWarning() << "[main][setup-sandbox] Failed to remove existing setup symlink/file:" << setupDriveSymlinkPath
                           << "- Error:" << strerror(errno);
                return 1;
            }
        }

        if (symlink(setupDriveMountPath.toStdString().c_str(), setupDriveSymlinkPath.toStdString().c_str()) != 0)
        {
            qWarning() << "[main][setup-sandbox] Failed to create setup symlink" << setupDriveSymlinkPath << "->" << setupDriveMountPath
                       << "- Error:" << strerror(errno);
            return 1;
        }

        qDebug() << "[main][setup-sandbox] Created setup symlink:" << setupDriveSymlinkPath << "->" << setupDriveMountPath;

        qDebug() << "[main][setup-sandbox] Initializing prefix with Proton run on hello_world.exe inside sandbox" << initPaths.compatBasePath;
        const int initStatus = runProtonCommandAndWait(initPaths, {QStringLiteral("run"), QStringLiteral("A:\\hello_world.exe")});
        qDebug() << "[main][setup-sandbox] Setup run exited with status:" << initStatus;
        if (initStatus != 0)
        {
            cleanupSetupDrive();
            return 1;
        }

        qDebug() << "[main][setup-sandbox] Waiting for 1 second before proceeding to ensure all file operations are completed...";
        sleep(1);

        cleanupSetupDrive();
        return 0;
    });

    qDebug() << "[main][setup-sandbox] Setup sandbox exited with status:" << setupSandbox.exitCode;

    if (!setupSandbox.success)
    {
        qWarning() << "[main][setup-sandbox] Setup sandbox failed:" << setupSandbox.errorMessage;
        return -1;
    }
    if (setupSandbox.exitCode != 0)
    {
        qWarning() << "[main][setup-sandbox] Setup sandbox exited with status:" << setupSandbox.exitCode;
        return setupSandbox.exitCode;
    }

    const SandboxResult gameSandbox = executeInPidSandbox([&]() -> int
    {
        if (!mountOverlayFilesystem(mergeDir, {gameDir, bepinexDir, modDir}, upperDir, workDir))
        {
            return 1;
        }

        if (!mountOverlayFilesystem(compatOverlayDir, {compatBaseDir}, compatOverlayUpperDir, compatOverlayWorkDir))
        {
            return 1;
        }

        const QString dosdevicesPath = compatOverlayDir + "pfx/dosdevices/";
        const QString symlinkPath = dosdevicesPath + "a:";
        const QString targetPath = "../../../merge";

        QDir dosdevicesDir(dosdevicesPath);
        if (!dosdevicesDir.exists())
        {
            qDebug() << "[main][game-sandbox] Creating dosdevices directory:" << dosdevicesPath;
            if (!dosdevicesDir.mkpath("."))
            {
                qWarning() << "[main][game-sandbox] Failed to create dosdevices directory:" << dosdevicesPath << "- Error:" << strerror(errno);
                return 1;
            }
        }

        if (QFileInfo(symlinkPath).isSymLink() || QFile::exists(symlinkPath))
        {
            qDebug() << "[main][game-sandbox] Removing existing file/symlink:" << symlinkPath;
            if (!QFile::remove(symlinkPath))
            {
                qWarning() << "[main][game-sandbox] Failed to remove existing symlink:" << symlinkPath
                           << "- Error:" << strerror(errno);
                return 1;
            }
        }

        if (symlink(targetPath.toStdString().c_str(), symlinkPath.toStdString().c_str()) != 0)
        {
            qWarning() << "[main][game-sandbox] Failed to create symlink" << symlinkPath << "->" << targetPath
                       << "- Error:" << strerror(errno);
            return 1;
        }

        qDebug() << "[main][game-sandbox] Created symlink:" << symlinkPath << "->" << targetPath;

        ProtonLaunchPaths protonPaths{
            .compatBasePath = QDir::cleanPath(QDir::current().absoluteFilePath(compatOverlayDir)),
            .steamInstallPath = QDir::cleanPath(QDir::home().filePath(".steam/steam")),
            .protonPath = QDir::cleanPath(QDir::home().filePath(".steam/steam/steamapps/common/Proton 10.0/proton")),
            .gameExePath = QDir::cleanPath(QDir(mergeDir).filePath("A Dance of Fire and Ice.exe")),
            .compatInstallPath = QDir::cleanPath(QDir::current().absoluteFilePath(mergeDir)),
        };

        printFileState("[main][game-sandbox] Game executable", protonPaths.gameExePath);
        qDebug() << "[main][game-sandbox] Launching Proton in runinprefix mode with:" << "A:\\A Dance of Fire and Ice.exe";
        return runProtonCommandAndWait(protonPaths, {QStringLiteral("runinprefix"), QStringLiteral("A:\\A Dance of Fire and Ice.exe")});
    });

    if (!gameSandbox.success)
    {
        qWarning() << "[main][game-sandbox] Game sandbox failed:" << gameSandbox.errorMessage;
        return -1;
    }

    qDebug() << "[main][game-sandbox] Game sandbox exited with status:" << gameSandbox.exitCode;
    return gameSandbox.exitCode;
}
