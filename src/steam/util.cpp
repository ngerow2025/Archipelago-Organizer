#include <QSysInfo>
#include <QOperatingSystemVersion>
#include "util.h"

SteamKit::EOSType GetOSType()
{
    // QOperatingSystemVersion has no Linux type, so handle it first via QSysInfo
    if (QSysInfo::kernelType() == QLatin1String("linux"))
    {
        const QStringList parts = QSysInfo::kernelVersion().split(QLatin1Char('.'));
        const int major = parts.value(0).toInt();
        const int minor = parts.value(1).toInt();

        switch (major)
        {
            case 2:
                switch (minor)
                {
                    case 2:  return SteamKit::EOSType::Linux22;
                    case 4:  return SteamKit::EOSType::Linux24;
                    case 6:  return SteamKit::EOSType::Linux26;
                    default: return SteamKit::EOSType::LinuxUnknown;
                }
            case 3:
                switch (minor)
                {
                    case 2:  return SteamKit::EOSType::Linux32;
                    case 5:  return SteamKit::EOSType::Linux35;
                    case 6:  return SteamKit::EOSType::Linux36;
                    case 10: return SteamKit::EOSType::Linux310;
                    case 16: return SteamKit::EOSType::Linux316;
                    case 18: return SteamKit::EOSType::Linux318;
                    default: return SteamKit::EOSType::Linux3x;
                }
            case 4:
                switch (minor)
                {
                    case 1:  return SteamKit::EOSType::Linux41;
                    case 4:  return SteamKit::EOSType::Linux44;
                    case 9:  return SteamKit::EOSType::Linux49;
                    case 14: return SteamKit::EOSType::Linux414;
                    case 19: return SteamKit::EOSType::Linux419;
                    default: return SteamKit::EOSType::Linux4x;
                }
            case 5:
                switch (minor)
                {
                    case 4:  return SteamKit::EOSType::Linux54;
                    case 10: return SteamKit::EOSType::Linux510;
                    default: return SteamKit::EOSType::Linux5x;
                }
            case 6:  return SteamKit::EOSType::Linux6x;
            case 7:  return SteamKit::EOSType::Linux7x;
            default: return SteamKit::EOSType::LinuxUnknown;
        }
    }

    const QOperatingSystemVersion osVer = QOperatingSystemVersion::current();
    const int major = osVer.majorVersion();
    const int minor = osVer.minorVersion();

    switch (osVer.type())
    {
        case QOperatingSystemVersion::Windows:
        {
            // Note: Win95/98/ME (Win32Windows) are not handled here as Qt does not
            // support those platforms. QOperatingSystemVersion covers Win32NT only.
            switch (major)
            {
                case 4: return SteamKit::EOSType::WinNT;
                case 5:
                    switch (minor)
                    {
                        case 0:  return SteamKit::EOSType::Win2000;
                        case 1:  return SteamKit::EOSType::WinXP;
                        case 2:  return SteamKit::EOSType::Win2003;
                        default: return SteamKit::EOSType::WinUnknown;
                    }
                case 6:
                    switch (minor)
                    {
                        case 0:  return SteamKit::EOSType::WinVista;  // Also Server 2008
                        case 1:  return SteamKit::EOSType::Windows7;  // Also Server 2008 R2
                        case 2:  return SteamKit::EOSType::Windows8;  // Also Server 2012
                        case 3:  return SteamKit::EOSType::Windows81; // Also Server 2012 R2
                        default: return SteamKit::EOSType::WinUnknown;
                    }
                case 10:
                {
                    // Disambiguate Win10/Win11 via the build number in the kernel version string
                    const QStringList parts = QSysInfo::kernelVersion().split(QLatin1Char('.'));
                    const int build = parts.value(2).toInt();
                    return build >= 22000 ? SteamKit::EOSType::Win11 : SteamKit::EOSType::Windows10;
                }
                default: return SteamKit::EOSType::WinUnknown;
            }
        }

        case QOperatingSystemVersion::MacOS:
        {
            // Note: unlike C#, Qt's QOperatingSystemVersion reports the actual macOS
            // marketing version (e.g. 14.x for Sonoma), not the Darwin kernel version,
            // so the major/minor mapping here differs from the original C# source.
            if (major == 10)
            {
                switch (minor)
                {
                    case 7:  return SteamKit::EOSType::MacOS107;  // Lion
                    case 8:  return SteamKit::EOSType::MacOS108;  // Mountain Lion
                    case 9:  return SteamKit::EOSType::MacOS109;  // Mavericks
                    case 10: return SteamKit::EOSType::MacOS1010; // Yosemite
                    case 11: return SteamKit::EOSType::MacOS1011; // El Capitan
                    case 12: return SteamKit::EOSType::MacOS1012; // Sierra
                    case 13: return SteamKit::EOSType::Macos1013; // High Sierra
                    case 14: return SteamKit::EOSType::Macos1014; // Mojave
                    case 15: return SteamKit::EOSType::Macos1015; // Catalina
                    default: return SteamKit::EOSType::MacOSUnknown;
                }
            }
            switch (major)
            {
                case 11: return SteamKit::EOSType::MacOS11; // Big Sur
                case 12: return SteamKit::EOSType::MacOS12; // Monterey
                case 13: return SteamKit::EOSType::MacOS13; // Ventura
                case 14: return SteamKit::EOSType::MacOS14; // Sonoma
                case 15: return SteamKit::EOSType::MacOS15; // Sequoia
                default: return SteamKit::EOSType::MacOSUnknown;
            }
        }

        default: return SteamKit::EOSType::Unknown;
    }
}