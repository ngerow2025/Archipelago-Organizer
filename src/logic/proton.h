#ifndef PROTON_H
#define PROTON_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <functional>

struct ProtonLaunchPaths {
    QString compatBasePath;
    QString steamInstallPath;
    QString protonPath;
    QString gameExePath;
    QString protonToolPath;   // e.g. .../steamapps/common/Proton 10.0
    QString sniperPath;       // e.g. .../steamapps/common/SteamLinuxRuntime_sniper
    QString gameInstallPath;  // e.g. .../steamapps/common/A Dance of Fire and Ice
    QString steamAppsPath;    // e.g. .../steamapps (or derive as steamInstallPath + "/steamapps")
    QString shaderCachePath;  // e.g. .../steamapps/shadercache/977950
};

inline const QMap<QString, std::function<QByteArray(const ProtonLaunchPaths&)>> kProtonEnvVars = {
    // --- existing ---
    {QStringLiteral("STEAM_COMPAT_DATA_PATH"),
     [](const ProtonLaunchPaths& paths) { return paths.compatBasePath.toUtf8(); }},
    {QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"),
     [](const ProtonLaunchPaths& paths) { return paths.steamInstallPath.toUtf8(); }},
    {QStringLiteral("PROTON_LOG"), [](const ProtonLaunchPaths&) { return QByteArrayLiteral("1"); }},
    {QStringLiteral("WINEDLLOVERRIDES"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("winhttp=n,b"); }},
    {QStringLiteral("WINEDEBUG"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("-all"); }},
    {QStringLiteral("SteamUser"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("nat_than"); }},
    {QStringLiteral("STEAM_COMPAT_APP_ID"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("977950"); }},

    // --- Steam IPC ---
    {QStringLiteral("Steam3Master"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("127.0.0.1:57343"); }},

    // --- Steam identity ---
    {QStringLiteral("SteamGameId"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("977950"); }},
    {QStringLiteral("SteamAppId"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("977950"); }},
    {QStringLiteral("SteamOverlayGameId"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("977950"); }},
    {QStringLiteral("SteamAppUser"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("nat_than"); }},
    {QStringLiteral("SteamClientLaunch"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("1"); }},
    {QStringLiteral("SteamEnv"), [](const ProtonLaunchPaths&) { return QByteArrayLiteral("1"); }},

    // --- Proton compat vars (add to kProtonEnvVars) ---
    {QStringLiteral("STEAM_COMPAT_PROTON"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("1"); }},
    {QStringLiteral("STEAM_COMPAT_FLAGS"),
     [](const ProtonLaunchPaths&) { return QByteArrayLiteral("search-cwd"); }},
    {QStringLiteral("SRT_LAUNCHER_SERVICE_ALONGSIDE_STEAM"),
     [](const ProtonLaunchPaths&) {
         return QByteArrayLiteral("com.steampowered.PressureVessel.LaunchAlongsideSteam");
     }},
    {QStringLiteral("STEAM_BASE_FOLDER"),
     [](const ProtonLaunchPaths& paths) { return paths.steamInstallPath.toUtf8(); }},
    {QStringLiteral("STEAM_COMPAT_INSTALL_PATH"),
     [](const ProtonLaunchPaths& paths) { return paths.gameInstallPath.toUtf8(); }},
    {QStringLiteral("STEAM_COMPAT_LIBRARY_PATHS"),
     [](const ProtonLaunchPaths& paths) { return paths.steamAppsPath.toUtf8(); }},
    {QStringLiteral("STEAM_COMPAT_TOOL_PATHS"),
     [](const ProtonLaunchPaths& paths) {
         return (paths.protonToolPath + QStringLiteral(":") + paths.sniperPath).toUtf8();
     }},
    {QStringLiteral("STEAM_COMPAT_MOUNTS"),
     [](const ProtonLaunchPaths& paths) {
         return (paths.protonToolPath + QStringLiteral(":") + paths.sniperPath).toUtf8();
     }},
    {QStringLiteral("STEAM_COMPAT_SHADER_PATH"),
     [](const ProtonLaunchPaths& paths) { return paths.shaderCachePath.toUtf8(); }},
    {QStringLiteral("STEAM_COMPAT_MEDIA_PATH"),
     [](const ProtonLaunchPaths& paths) {
         return (paths.shaderCachePath + QStringLiteral("/fozmediav1")).toUtf8();
     }},
    {QStringLiteral("STEAM_COMPAT_TRANSCODED_MEDIA_PATH"),
     [](const ProtonLaunchPaths& paths) { return paths.shaderCachePath.toUtf8(); }},
};

void applyProtonEnvironment(const ProtonLaunchPaths& paths);
int  runProtonCommandAndWait(const ProtonLaunchPaths& paths, const QStringList& protonArgs);

#endif  // PROTON_H
