#include "main.h"

#include <QDir>
#include <QLoggingCategory>

#include "../SteamKit/SteamKit2/SteamKit2/bin/Release/net8.0/linux-x64/SteamKit2.h"
#include "logic/mounts.h"
#include "logic/proton.h"
#include "logic/sandbox.h"
#include "logic/util.h"
#include "mainwindow.h"
#include "sys/mount.h"
#include "sys/wait.h"

static intptr_t steamClient;
static intptr_t steamUser;

void OnConnectedCallback(intptr_t callback) {
    qDebug() << "Connected to Steam!";
    intptr_t authSession = SteamClient_Authentication_BeginAuthSessionViaQRAsync(steamClient);

    AuthSession_SetChallengeURLChangedCallback(authSession, reinterpret_cast<intptr_t>(+[](void) {
        qDebug() << "Challenge URL changed!";
    }));

    AuthPollResultNative authPollResult = AuthSession_PollingWaitForResultAsync(authSession);

    qDebug() << "Logging in as " << authPollResult.account_name;

    SteamUser_LogOn(steamUser, CreateLogOnDetails(
        authPollResult.account_name,
        authPollResult.refresh_token
    ));
}

void OnDisconnectedCallback(intptr_t callback) {
    qDebug() << "Disconnected from Steam!";
}

void OnLoggedOnCallback(intptr_t callback) {
    qDebug() << "Logged on to Steam!";
    qDebug() << "Result: " << LoggedOnCallbackData_GetResult(callback);
}

void OnLoggedOffCallback(intptr_t callback) {
    qDebug() << "Logged off from Steam!";
}

int main(int argc, char* argv[]) {
    steamClient = CreateSteamClient();
    intptr_t callbackManager = CreateCallbackManager(steamClient);
    CallbackManagerSubscribeConnectedCallback(callbackManager, reinterpret_cast<intptr_t>(OnConnectedCallback));
    CallbackManagerSubscribeDisconnectedCallback(callbackManager, reinterpret_cast<intptr_t>(OnDisconnectedCallback));
    CallbackManagerSubscribeLoggedOnCallback(callbackManager, reinterpret_cast<intptr_t>(OnLoggedOnCallback));
    CallbackManagerSubscribeLoggedOffCallback(callbackManager, reinterpret_cast<intptr_t>(OnLoggedOffCallback));
    steamUser = GetSteamUserHandler(steamClient);

    SteamClientConnect(steamClient);

    while(true){
        qDebug() << "Running callbacks...";
        CallbackManagerRunCallbacks(callbackManager);
        sleep(1);
    }

    return 0;

    {
        int retVal;
        if ((retVal = activateUserNamespaceSandbox()) != 0) {
            return retVal;
        }
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
    if (!steamDirObj.exists()) {
        qWarning() << "[main][environment] Steam directory does not exist:" << steamDir;
        return -1;
    } else {
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

    QString compatBaseDir = "./compat_base/";
    QString compatOverlayDir = "./compat_base_overlay/";
    QString compatOverlayUpperDir = "./compat_base_overlay_upper/";
    QString compatOverlayWorkDir = "./compat_base_overlay_work/";

    // ensure that work is empty
    QDir workDirObj(workDir);
    if (workDirObj.exists()) {
        qDebug() << "[main][filesystem-prep] Cleaning work directory:" << workDir;
        workDirObj.removeRecursively();
    }
    workDirObj.mkpath(".");

    if (!clearDirectoryIfExists(compatBaseDir)) {
        return -1;
    }

    if (!clearDirectoryIfExists(compatOverlayDir) ||
        !clearDirectoryIfExists(compatOverlayUpperDir) ||
        !clearDirectoryIfExists(compatOverlayWorkDir)) {
        return -1;
    }

    if (!clearDirectoryIfExists(setupDriveDir)) {
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
    if (!bepinexDirObj.exists()) {
        qWarning() << "[main][filesystem-prep] BepInEx directory does not exist:" << bepinexDir;
        return -1;
    }

    QDir modDirObj(modDir);
    if (!modDirObj.exists()) {
        qWarning() << "[main][filesystem-prep] Mod directory does not exist:" << modDir;
        return -1;
    }

    const SandboxResult setupSandbox = executeInPidSandbox([&]() -> int {
        ProtonLaunchPaths initPaths{
            .compatBasePath = QDir::cleanPath(QDir::current().absoluteFilePath(compatBaseDir)),
            .steamInstallPath = QDir::cleanPath(QDir::home().filePath(".steam/steam")),
            .protonPath = QDir::cleanPath(
                QDir::home().filePath(".steam/steam/steamapps/common/Proton 10.0/proton")),
            .gameExePath = QDir::cleanPath(QDir::current().absoluteFilePath("hello_world.exe")),
            .protonToolPath =
                QDir::cleanPath(QDir::home().filePath(".steam/steam/steamapps/common/Proton 10.0")),
            .sniperPath = QDir::cleanPath(
                QDir::home().filePath(".steam/steam/steamapps/common/SteamLinuxRuntime_sniper")),
            .gameInstallPath = QDir::cleanPath(QDir::current().absoluteFilePath(gameDir)),
            .steamAppsPath = QDir::cleanPath(QDir::home().filePath(".steam/steam/steamapps")),
            .shaderCachePath =
                QDir::cleanPath(QDir::home().filePath(".steam/steam/steamapps/shadercache/977950")),
        };

        const QString setupExecutableSource =
            QDir::cleanPath(QDir::current().absoluteFilePath("hello_world.exe"));
        const QString setupDriveMountPath =
            QDir::cleanPath(QDir::current().absoluteFilePath(setupDriveDir));
        const QString setupDriveExecutablePath =
            QDir(setupDriveMountPath).filePath("hello_world.exe");
        const QString setupDosdevicesPath = compatBaseDir + "pfx/dosdevices/";
        const QString setupDriveSymlinkPath = setupDosdevicesPath + "a:";

        const auto cleanupSetupDrive = [&]() {
            if (QFileInfo(setupDriveSymlinkPath).isSymLink() ||
                QFile::exists(setupDriveSymlinkPath)) {
                QFile::remove(setupDriveSymlinkPath);
            }

            if (umount(setupDriveExecutablePath.toStdString().c_str()) != 0) {
                qWarning() << "[main][setup-sandbox] Failed to unmount setup "
                              "executable bind mount:"
                           << strerror(errno);
            }

            if (!clearDirectoryIfExists(setupDriveDir)) {
                qWarning() << "[main][setup-sandbox] Failed to clear setup drive directory:"
                           << setupDriveDir;
            }
        };

        QDir setupDosdevicesDir(setupDosdevicesPath);
        if (!setupDosdevicesDir.exists()) {
            qDebug() << "[main][setup-sandbox] Creating setup dosdevices directory:"
                     << setupDosdevicesPath;
            if (!setupDosdevicesDir.mkpath(".")) {
                qWarning() << "[main][setup-sandbox] Failed to create setup dosdevices "
                              "directory:"
                           << setupDosdevicesPath << "- Error:" << strerror(errno);
                return 1;
            }
        }

        QFile setupDriveExecutablePlaceholder(setupDriveExecutablePath);
        if (!setupDriveExecutablePlaceholder.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "[main][setup-sandbox] Failed to create setup drive mount point:"
                       << setupDriveExecutablePath
                       << "- Error:" << setupDriveExecutablePlaceholder.errorString();
            return 1;
        }
        setupDriveExecutablePlaceholder.close();

        if (!mountBindFilesystem(setupExecutableSource, setupDriveExecutablePath)) {
            return 1;
        }

        if (QFileInfo(setupDriveSymlinkPath).isSymLink() || QFile::exists(setupDriveSymlinkPath)) {
            qDebug() << "[main][setup-sandbox] Removing existing setup symlink/file:"
                     << setupDriveSymlinkPath;
            if (!QFile::remove(setupDriveSymlinkPath)) {
                qWarning() << "[main][setup-sandbox] Failed to remove existing setup "
                              "symlink/file:"
                           << setupDriveSymlinkPath << "- Error:" << strerror(errno);
                return 1;
            }
        }

        if (symlink(setupDriveMountPath.toStdString().c_str(),
                    setupDriveSymlinkPath.toStdString().c_str()) != 0) {
            qWarning() << "[main][setup-sandbox] Failed to create setup symlink"
                       << setupDriveSymlinkPath << "->" << setupDriveMountPath
                       << "- Error:" << strerror(errno);
            return 1;
        }

        qDebug() << "[main][setup-sandbox] Created setup symlink:" << setupDriveSymlinkPath << "->"
                 << setupDriveMountPath;

        qDebug() << "[main][setup-sandbox] Initializing prefix with Proton run on "
                    "hello_world.exe inside sandbox"
                 << initPaths.compatBasePath;
        const int initStatus = runProtonCommandAndWait(
            initPaths, {QStringLiteral("run"), QStringLiteral("A:\\hello_world.exe")});
        qDebug() << "[main][setup-sandbox] Setup run exited with status:" << initStatus;
        if (initStatus != 0) {
            cleanupSetupDrive();
            return 1;
        }

        qDebug() << "[main][setup-sandbox] Waiting for 1 second before proceeding "
                    "to ensure all file operations are completed...";
        sleep(1);

        cleanupSetupDrive();
        return 0;
    });

    qDebug() << "[main][setup-sandbox] Setup sandbox exited with status:" << setupSandbox.exitCode;

    if (!setupSandbox.success) {
        qWarning() << "[main][setup-sandbox] Setup sandbox failed:" << setupSandbox.errorMessage;
        return -1;
    }
    if (setupSandbox.exitCode != 0) {
        qWarning() << "[main][setup-sandbox] Setup sandbox exited with status:"
                   << setupSandbox.exitCode;
        return setupSandbox.exitCode;
    }

    const SandboxResult gameSandbox = executeInPidSandbox([&]() -> int {
        if (!mountOverlayFilesystem(mergeDir, {gameDir, bepinexDir, modDir}, upperDir, workDir)) {
            return 1;
        }

        if (!mountOverlayFilesystem(compatOverlayDir, {compatBaseDir}, compatOverlayUpperDir,
                                    compatOverlayWorkDir)) {
            return 1;
        }

        const QString dosdevicesPath = compatOverlayDir + "pfx/dosdevices/";
        const QString symlinkPath = dosdevicesPath + "a:";
        const QString targetPath = "../../../merge";

        QDir dosdevicesDir(dosdevicesPath);
        if (!dosdevicesDir.exists()) {
            qDebug() << "[main][game-sandbox] Creating dosdevices directory:" << dosdevicesPath;
            if (!dosdevicesDir.mkpath(".")) {
                qWarning() << "[main][game-sandbox] Failed to create dosdevices directory:"
                           << dosdevicesPath << "- Error:" << strerror(errno);
                return 1;
            }
        }

        if (QFileInfo(symlinkPath).isSymLink() || QFile::exists(symlinkPath)) {
            qDebug() << "[main][game-sandbox] Removing existing file/symlink:" << symlinkPath;
            if (!QFile::remove(symlinkPath)) {
                qWarning() << "[main][game-sandbox] Failed to remove existing symlink:"
                           << symlinkPath << "- Error:" << strerror(errno);
                return 1;
            }
        }

        if (symlink(targetPath.toStdString().c_str(), symlinkPath.toStdString().c_str()) != 0) {
            qWarning() << "[main][game-sandbox] Failed to create symlink" << symlinkPath << "->"
                       << targetPath << "- Error:" << strerror(errno);
            return 1;
        }

        qDebug() << "[main][game-sandbox] Created symlink:" << symlinkPath << "->" << targetPath;

        ProtonLaunchPaths protonPaths{
            .compatBasePath = QDir::cleanPath(QDir::current().absoluteFilePath(compatOverlayDir)),
            .steamInstallPath = QDir::cleanPath(QDir::home().filePath(".steam/steam")),
            .protonPath = QDir::cleanPath(
                QDir::home().filePath(".steam/steam/steamapps/common/Proton 10.0/proton")),
            .gameExePath = QDir::cleanPath(QDir(mergeDir).filePath("A Dance of Fire and Ice.exe")),
            .protonToolPath =
                QDir::cleanPath(QDir::home().filePath(".steam/steam/steamapps/common/Proton 10.0")),
            .sniperPath = QDir::cleanPath(
                QDir::home().filePath(".steam/steam/steamapps/common/SteamLinuxRuntime_sniper")),
            .gameInstallPath = QDir::cleanPath(QDir::current().absoluteFilePath(gameDir)),
            .steamAppsPath = QDir::cleanPath(QDir::home().filePath(".steam/steam/steamapps")),
            .shaderCachePath =
                QDir::cleanPath(QDir::home().filePath(".steam/steam/steamapps/shadercache/977950")),
        };

        printFileState("[main][game-sandbox] Game executable", protonPaths.gameExePath);
        qDebug() << "[main][game-sandbox] Launching Proton in runinprefix mode with:"
                 << "A:\\A Dance of Fire and Ice.exe";
        return runProtonCommandAndWait(
            protonPaths,
            {QStringLiteral("runinprefix"), QStringLiteral("A:\\A Dance of Fire and Ice.exe")});
    });

    if (!gameSandbox.success) {
        qWarning() << "[main][game-sandbox] Game sandbox failed:" << gameSandbox.errorMessage;
        return -1;
    }

    qDebug() << "[main][game-sandbox] Game sandbox exited with status:" << gameSandbox.exitCode;
    return gameSandbox.exitCode;
}
