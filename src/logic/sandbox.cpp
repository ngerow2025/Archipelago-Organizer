#include "sandbox.h"

#include <dirent.h>
#include <linux/prctl.h>
#include <qdebug.h>
#include <qfile.h>
#include <signal.h>
#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>

// --- GLOBALS FOR SIGNAL HANDLING, these are implicitly copied per thread/process ---
static pid_t            g_workerPid = -1;
static std::atomic<int> g_pendingSignal{0};

void atomicSignalHandler(int signum) {
    g_pendingSignal.store(signum);
}

QString processNameForPid(pid_t pid) {
    QFile commFile(QStringLiteral("/proc/%1/comm").arg(pid));
    if (!commFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("<unknown>");
    }

    const QString name = QString::fromUtf8(commFile.readAll()).trimmed();
    return name.isEmpty() ? QStringLiteral("<unknown>") : name;
}

std::vector<pid_t> getActivePidsInsideNamespace() {
    std::vector<pid_t> pids;
    DIR*               dir = opendir("/proc");
    if (!dir) {
        qWarning() << "[PID 1] Failed to open /proc to scan for alive processes.";
        return pids;
    }

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_type == DT_DIR) {
            QString name = ent->d_name;
            bool    ok;
            pid_t   pid = name.toInt(&ok);
            // Ignore PID 1 (itself). Any other numeric folder is an active process.
            if (ok && pid > 1) {
                pids.push_back(pid);
            }
        }
    }
    closedir(dir);
    return pids;
}

void printSandboxProcessSnapshot(const QString& label) {
    std::vector<pid_t> pids = getActivePidsInsideNamespace();
    std::sort(pids.begin(), pids.end());

    qDebug() << "[PID 1]" << label << "active sandbox processes:";
    if (pids.empty()) {
        qDebug() << "[PID 1]   (none)";
        return;
    }

    qDebug() << "[PID 1]   PID  COMMAND";
    for (pid_t pid : pids) {
        qDebug().noquote()
            << QStringLiteral("[PID 1]   %1  %2").arg(pid, 4).arg(processNameForPid(pid));
    }
}

// --- THE SINGLE ENTRY POINT UTILITY ---
SandboxResult executeInPidSandbox(const std::function<int()>& lambdaTask) {
    g_pendingSignal.store(0);
    pid_t parentPid = getpid();
    pid_t launcherPid = fork();

    if (launcherPid < 0) return {false, -1, QString("Fork failed.")};

    if (launcherPid == 0) {
        prctl(PR_SET_PDEATHSIG, SIGKILL);
        if (getppid() != parentPid) _exit(EXIT_FAILURE);

        if (unshare(CLONE_NEWPID | CLONE_NEWNS) != 0) _exit(EXIT_FAILURE);
        if (mount("none", "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) _exit(EXIT_FAILURE);

        pid_t rootPid = fork();
        if (rootPid < 0) _exit(EXIT_FAILURE);

        if (rootPid == 0) {
            // ==========================================
            // FORK 2: THE NAMESPACE ROOT (PID 1)
            // ==========================================
            prctl(PR_SET_PDEATHSIG, SIGKILL);
            if (getppid() != 0) _exit(EXIT_FAILURE);
            if (mount("proc", "/proc", "proc", 0, NULL) != 0) _exit(EXIT_FAILURE);

            struct sigaction sa;
            sa.sa_handler = atomicSignalHandler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGTERM, &sa, nullptr);
            sigaction(SIGINT, &sa, nullptr);

            g_workerPid = fork();
            if (g_workerPid < 0) _exit(EXIT_FAILURE);

            if (g_workerPid == 0) {
                // ==========================================
                // FORK 3: THE WORKER PROCESS (PID 2)
                // ==========================================
                if (getppid() != 1) _exit(EXIT_FAILURE);

                signal(SIGTERM, SIG_DFL);
                signal(SIGINT, SIG_DFL);

                _exit(lambdaTask());
            }

            // ==========================================
            // PID 1: THE EVENT LOOP
            // ==========================================
            bool       workerDeadGracePeriod = false;
            bool       externalShutdownInitiated = false;
            auto       gracePeriodStart = std::chrono::steady_clock::now();
            const auto GRACE_PERIOD = std::chrono::seconds(10);
            int        finalExitStatus = EXIT_FAILURE;

            while (true) {
                // 1. Check for external shutdown signals
                int sig = g_pendingSignal.exchange(0);
                if (sig != 0 && !externalShutdownInitiated && !workerDeadGracePeriod) {
                    qDebug().noquote()
                        << "[PID 1] External signal received. Broadcasting to sandbox...";
                    kill(-1, sig);
                    externalShutdownInitiated = true;
                    gracePeriodStart = std::chrono::steady_clock::now();
                }

                // 2. Reap Processes
                // Use WNOHANG if we are waiting for timers, otherwise block normally.
                int   options = (workerDeadGracePeriod || externalShutdownInitiated) ? WNOHANG : 0;
                int   status = 0;
                pid_t reapedPid = waitpid(-1, &status, options);

                if (reapedPid > 0) {
                    if (reapedPid == g_workerPid) {
                        qDebug() << "[PID 1] Main worker exited. Initiating 10-second "
                                    "grace period for remaining background processes.";

                        // Save the worker's exit status so PID 1 can return it later
                        if (WIFEXITED(status))
                            finalExitStatus = WEXITSTATUS(status);
                        else if (WIFSIGNALED(status))
                            finalExitStatus = 128 + WTERMSIG(status);

                        workerDeadGracePeriod = true;
                        gracePeriodStart = std::chrono::steady_clock::now();

                        printSandboxProcessSnapshot(
                            QStringLiteral("Process snapshot #1 before SIGINT"));
                        sleep(1);
                        printSandboxProcessSnapshot(
                            QStringLiteral("Process snapshot #2 before SIGINT"));

                        // Broadcast SIGINT to give remaining processes a chance to clean up
                        kill(-1, SIGINT);
                    } else {
                        qDebug() << "[PID 1] Successfully reaped background process PID:"
                                 << reapedPid;
                    }
                } else if (reapedPid == 0) {
                    // Check our timeout
                    if (workerDeadGracePeriod || externalShutdownInitiated) {
                        if (std::chrono::steady_clock::now() - gracePeriodStart > GRACE_PERIOD) {
                            qWarning() << "------------------------------------------";
                            qWarning() << "[PID 1] 10-second grace period expired!";

                            std::vector<pid_t> alivePids = getActivePidsInsideNamespace();
                            for (pid_t p : alivePids) {
                                qWarning() << "[PID 1] Process" << p
                                           << "is still alive! Sending targeted SIGKILL.";
                                kill(p, SIGKILL);
                            }
                            qWarning() << "------------------------------------------";

                            // Exit PID 1, letting the kernel act as a final fallback kill
                            _exit(finalExitStatus);
                        }
                        usleep(50000);  // 50ms polling rest
                    }
                } else {
                    if (errno == EINTR) continue;
                    if (errno == ECHILD) {
                        qDebug() << "[PID 1] All sandbox processes have cleanly exited. "
                                    "Shutting down namespace.";
                        if (workerDeadGracePeriod) _exit(finalExitStatus);
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
        while (true) {
            int sig = g_pendingSignal.exchange(0);
            if (sig != 0) kill(rootPid, sig);

            if (waitpid(rootPid, &rootStatus, 0) == -1) {
                if (errno == EINTR) continue;
                _exit(EXIT_FAILURE);
            }
            break;
        }

        if (WIFEXITED(rootStatus)) _exit(WEXITSTATUS(rootStatus));
        _exit(128 + WTERMSIG(rootStatus));
    }

    // ==========================================
    // BACK IN MAIN QT PROCESS
    // ==========================================
    int launcherStatus = 0;
    while (waitpid(launcherPid, &launcherStatus, 0) == -1) {
        if (errno == EINTR) continue;
        return {false, -1, QString("waitpid failed: %1").arg(strerror(errno))};
    }

    if (WIFEXITED(launcherStatus)) return {true, WEXITSTATUS(launcherStatus), QString()};
    return {false, -1, QString("Launcher process failed.")};
}

void printCapability(const char* capName, cap_value_t cap) {
    cap_t caps = cap_get_proc();
    if (caps == nullptr) {
        qWarning() << "[main][bootstrap] Failed to get capabilities:" << strerror(errno);
        return;
    }

    cap_flag_value_t capValue;
    if (cap_get_flag(caps, cap, CAP_EFFECTIVE, &capValue) != 0) {
        qWarning() << "[main][bootstrap] Failed to get" << capName
                   << "capability:" << strerror(errno);
        cap_free(caps);
        return;
    }

    qDebug() << "[main][bootstrap]" << capName << ":" << ((capValue == CAP_SET) ? "Yes" : "No");
    cap_free(caps);
}

int activateUserNamespaceSandbox() {
    // print out the current UID and GID to verify that we are now root inside
    // the namespace
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

    // start by creating a user namespace so that this process executes with
    // pseudo-root privileges, allowing us to mount the overlay filesystem
    // without root.
    if (unshare(CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWIPC) != 0) {
        qWarning() << "Failed to unshare user namespace:" << strerror(errno);
        return -1;
    }
    qDebug() << "[main][bootstrap] User namespace unshared successfully.";

    // print out the current PID
    qDebug() << "[main][bootstrap] Current PID:" << getpid();

    // read current capabilities using capget to verify that we have the
    // necessary capabilities to perform the mount spesifically check
    // CAP_SETUID, CAP_SETGID, CAP_SETFCAP
    printCapability("CAP_SETUID", CAP_SETUID);
    printCapability("CAP_SETGID", CAP_SETGID);
    printCapability("CAP_SETFCAP", CAP_SETFCAP);

    // edit the user mapping in /proc/self/uid_map to map the current user to
    // root inside the namespace
    QString uidMap = QString("0 %1 1").arg(uid);
    QString gidMap = QString("0 %1 1").arg(gid);
    QFile   uidMapFile("/proc/self/uid_map");
    if (!uidMapFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open uid_map for writing:" << strerror(errno);
        return -1;
    }
    if (uidMapFile.write(uidMap.toUtf8()) == -1) {
        qWarning() << "Failed to write to uid_map:" << strerror(errno);
        return -1;
    }
    uidMapFile.close();

    // write "deny" to /proc/self/setgroups to prevent the kernel from rejecting
    // our GID mapping
    QFile setGroupsFile("/proc/self/setgroups");
    if (!setGroupsFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open setgroups for writing:" << strerror(errno);
        return -1;
    }
    if (setGroupsFile.write("deny") == -1) {
        qWarning() << "Failed to write to setgroups:" << strerror(errno);
        return -1;
    }
    setGroupsFile.close();

    QFile gidMapFile("/proc/self/gid_map");
    if (!gidMapFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open gid_map for writing:" << strerror(errno);
        return -1;
    }
    if (gidMapFile.write(gidMap.toUtf8()) == -1) {
        qWarning() << "Failed to write to gid_map:" << strerror(errno);
        return -1;
    }
    gidMapFile.close();

    // print out the current UID and GID to verify that we are now root inside
    // the namespace
    qDebug() << "[main][bootstrap] Current UID:" << getuid() << "(should be 0)";
    qDebug() << "[main][bootstrap] Current GID:" << getgid() << "(should be 0)";

    return 0;
}
