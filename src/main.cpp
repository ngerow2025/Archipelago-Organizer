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

// Global variable to hold the child's process group ID
volatile sig_atomic_t game_pgid = 0;

// The function that runs when Ctrl+C is pressed
void handle_signal(int sig) {
    if (game_pgid > 0) {
        // The negative sign is the secret sauce! 
        // kill(-PID, SIGTERM) sends the signal to the ENTIRE process group.
        qDebug() << "\nSignal" << sig << "caught! Sending SIGTERM to process group" << game_pgid;
        kill(-game_pgid, SIGTERM);
        
        // Optional: If you want to be ruthless, you could use SIGKILL (9) 
        // but SIGTERM (15) allows Wine to try and save state gracefully.
    }
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
    if (!sourceDir.exists()) {
        qWarning() << "[FAILURE] Source directory does not exist:" << sourcePath << "- Error:" << strerror(errno);
        return false;
    }

    QDir destDir(destPath);
    if (!destDir.exists()) {
        qDebug() << "[MKDIR] Creating destination directory:" << destPath;
        if (!destDir.mkpath(".")) {
            qWarning() << "[FAILURE] Failed to create destination directory:" << destPath << "- Error:" << strerror(errno);
            return false;
        }
    }

    const QFileInfoList entries = sourceDir.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System
    );

    for (const QFileInfo &entry : entries) {
        const QString srcEntry  = entry.absoluteFilePath();
        const QString destEntry = destPath + QDir::separator() + entry.fileName();

        if (entry.isDir()) {
            if (!copyRecursively(srcEntry, destEntry)) {
                qWarning() << "[FAILURE] Recursive copy failed for directory:" << srcEntry;
                return false;
            }
        } else {
            if (QFile::exists(destEntry)) {
                if (filesAreIdentical(srcEntry, destEntry)) {
                    qDebug() << "[IDENTICAL]  Skipping:" << destEntry;
                    continue;
                }

                qDebug() << "[OVERWRITE]  Replacing:" << destEntry;
                if (!QFile::remove(destEntry)) {
                    QFileInfo fileInfo(destEntry);
                    qWarning() << "[FAILURE] Failed to remove existing file:" << destEntry 
                               << "- Writable:" << fileInfo.isWritable() 
                               << "- Error:" << strerror(errno);
                    return false;
                }
            }
            qDebug() << "[COPY]  Copying:" << srcEntry << "to" << destEntry;
            if (!QFile::copy(srcEntry, destEntry)) {
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

void dropPrivileges(const char* username) {
    passwd* pw = getpwnam(username);

    if (!pw) {
        perror("getpwnam");
        exit(1);
    }

    if (setgid(pw->pw_gid) != 0) {
        perror("setgid");
        exit(1);
    }

    if (setuid(pw->pw_uid) != 0) {
        perror("setuid");
        exit(1);
    }

    qputenv("HOME", pw->pw_dir);
    qputenv("USER", pw->pw_name);
    qputenv("LOGNAME", pw->pw_name);
}


int main(int argc, char *argv[])
{
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
        qWarning() << "Steam directory does not exist:" << steamDir;
        return -1;
    }
    else
    {
        qDebug() << "Steam directory found:" << steamDir;
        QStringList folders = steamDirObj.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        qDebug() << "Found folders in Steam directory:" << folders;
    }

    QString basePath = "./Game/";
    QString upperDir = "./upper/";
    QString mergeDir = "./merge/";
    QString workDir = "./work/";

    // ensure that work is empty
    QDir workDirObj(workDir);
    if (workDirObj.exists())
    {
        qDebug() << "Cleaning work directory:" << workDir;
        workDirObj.removeRecursively();
    }
    workDirObj.mkpath(".");

    //ensure that each required dir exists
    QDir().mkpath(basePath);
    QDir().mkpath(upperDir);
    QDir().mkpath(mergeDir);

    // Change ownership of overlay directories to "birb" user
    struct passwd *birb_pwd = getpwnam("birb");
    if (birb_pwd == nullptr) {
        qWarning() << "[FAILURE] Failed to get pwd entry for user 'birb' - Error:" << strerror(errno);
        return -1;
    }

    QStringList dirs = {basePath, upperDir, mergeDir, workDir};
    for (const QString &dir : dirs) {
        if (chown(dir.toStdString().c_str(), birb_pwd->pw_uid, birb_pwd->pw_gid) != 0) {
            qWarning() << "[FAILURE] Failed to change ownership of" << dir << "to 'birb' (uid:" << birb_pwd->pw_uid << ", gid:" << birb_pwd->pw_gid << ") - Error:" << strerror(errno);
            return -1;
        }
        qDebug() << "[CHOWN] Changed ownership of" << dir << "to" << birb_pwd->pw_name;
    }

    QString opts = QString("lowerdir=%1,upperdir=%2,workdir=%3")
            .arg(basePath)
            .arg(upperDir)
            .arg(workDir);
    qDebug() << "Mounting overlay filesystem with options:" << opts;


    if (mount("overlay", mergeDir.toStdString().c_str(), "overlay",
              0, opts.toStdString().c_str()) != 0)
    {
        qWarning() << "Failed to mount overlay filesystem:" << strerror(errno);
        return -1;
    }

    dropPrivileges("birb");

    // Copy BepInEx to merged directory first
    QDir bepinexDirObj("./BepInEx_win_x64_5.4.23.5/");
    if (!bepinexDirObj.exists()) {
        qWarning() << "BepInEx directory does not exist:" << "./BepInEx_win_x64_5.4.23.5/";
        return -1;
    } else {
        qDebug() << "Copying BepInEx to merged directory...";
        copyRecursively("./BepInEx_win_x64_5.4.23.5/", mergeDir);
        qDebug() << "Finished copying BepInEx files to merged directory.";
    }

    //copy the files and directories from the mod directory into merged
    QDir modDirObj("./mod/ADOFAI_AP-Mod/");
    if (!modDirObj.exists())
    {
        qWarning() << "Mod directory does not exist:" << "./mod/ADOFAI_AP-Mod/";
        return -1;
    } else {
        qDebug() << "Copying mod to merged directory...";
        copyRecursively("./mod/ADOFAI_AP-Mod/", mergeDir);
        qDebug() << "Copied mod files to merged directory.";
    }

    // 1. Register the signal handlers before forking
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);  // Catch Ctrl+C
    sigaction(SIGTERM, &sa, nullptr); // Catch generic termination

    //now fork and exec the game inside the merged directory
    pid_t pid = fork();
    if (pid == 0) {
        // detach from parent process group and become group leader
        setpgid(0, 0);

        //set some env vars
        
        //export STEAM_COMPAT_DATA_PATH="$(pwd)/compat_base/"
        //this must be an absolute path
        QString compatBasePath = QDir::current().absoluteFilePath("compat_base");
        qDebug() << "Setting STEAM_COMPAT_DATA_PATH to:" << compatBasePath;
        qputenv("STEAM_COMPAT_DATA_PATH", compatBasePath.toUtf8());

        //export STEAM_COMPAT_CLIENT_INSTALL_PATH="$HOME/.steam/steam"
        QString steamInstallPath = QDir::home().filePath(".steam/steam");
        qDebug() << "Setting STEAM_COMPAT_CLIENT_INSTALL_PATH to:" << steamInstallPath;
        qputenv("STEAM_COMPAT_CLIENT_INSTALL_PATH", steamInstallPath.toUtf8());

        QString protonPath = QDir::home().filePath(".steam/steam/steamapps/common/Proton 10.0/proton");
        QString gameExePath = QDir(mergeDir).filePath("A Dance of Fire and Ice.exe");

        qDebug() << "Executing game with Proton:" << protonPath << "and game exe:" << gameExePath;
        execl(protonPath.toStdString().c_str(), protonPath.toStdString().c_str(), "run", gameExePath.toStdString().c_str(), nullptr);
        
        qWarning() << "Failed to exec game:" << strerror(errno);
        return -1;
    } else if (pid > 0) {
        game_pgid = getpgid(pid);
        int status;

        while(true){
            pid_t waitPid = waitpid(pid, &status, 0);

            if (waitPid == -1) {
                if (errno == EINTR) {
                    // waitpid was interrupted by our signal handler catching Ctrl+C.
                    // The handler has already fired the kill signal to the child group.
                    // Loop again to wait for them to finish dying.
                    continue; 
                } else {
                    // A real error occurred
                    qWarning() << "waitpid failed:" << strerror(errno);
                    break;
                }
            }
            break;
        }

        qDebug() << "Game process exited with status:" << status;

        //unmount the overlay filesystem
        if (umount(mergeDir.toStdString().c_str()) != 0)
        {
            qWarning() << "Failed to unmount overlay filesystem:" << strerror(errno);
            return -1;
        }
    } else {
        qWarning() << "Failed to fork:" << strerror(errno);
        return -1;
    }



}


