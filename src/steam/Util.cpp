#include <QSysInfo>
#include <QOperatingSystemVersion>
#include "Util.h"

steam::lang::EOSType GetOSType()
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
                    case 2:  return steam::lang::EOSType::Linux22;
                    case 4:  return steam::lang::EOSType::Linux24;
                    case 6:  return steam::lang::EOSType::Linux26;
                    default: return steam::lang::EOSType::LinuxUnknown;
                }
            case 3:
                switch (minor)
                {
                    case 2:  return steam::lang::EOSType::Linux32;
                    case 5:  return steam::lang::EOSType::Linux35;
                    case 6:  return steam::lang::EOSType::Linux36;
                    case 10: return steam::lang::EOSType::Linux310;
                    case 16: return steam::lang::EOSType::Linux316;
                    case 18: return steam::lang::EOSType::Linux318;
                    default: return steam::lang::EOSType::Linux3x;
                }
            case 4:
                switch (minor)
                {
                    case 1:  return steam::lang::EOSType::Linux41;
                    case 4:  return steam::lang::EOSType::Linux44;
                    case 9:  return steam::lang::EOSType::Linux49;
                    case 14: return steam::lang::EOSType::Linux414;
                    case 19: return steam::lang::EOSType::Linux419;
                    default: return steam::lang::EOSType::Linux4x;
                }
            case 5:
                switch (minor)
                {
                    case 4:  return steam::lang::EOSType::Linux54;
                    case 10: return steam::lang::EOSType::Linux510;
                    default: return steam::lang::EOSType::Linux5x;
                }
            case 6:  return steam::lang::EOSType::Linux6x;
            case 7:  return steam::lang::EOSType::Linux7x;
            default: return steam::lang::EOSType::LinuxUnknown;
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
                case 4: return steam::lang::EOSType::WinNT;
                case 5:
                    switch (minor)
                    {
                        case 0:  return steam::lang::EOSType::Win2000;
                        case 1:  return steam::lang::EOSType::WinXP;
                        case 2:  return steam::lang::EOSType::Win2003;
                        default: return steam::lang::EOSType::WinUnknown;
                    }
                case 6:
                    switch (minor)
                    {
                        case 0:  return steam::lang::EOSType::WinVista;  // Also Server 2008
                        case 1:  return steam::lang::EOSType::Windows7;  // Also Server 2008 R2
                        case 2:  return steam::lang::EOSType::Windows8;  // Also Server 2012
                        case 3:  return steam::lang::EOSType::Windows81; // Also Server 2012 R2
                        default: return steam::lang::EOSType::WinUnknown;
                    }
                case 10:
                {
                    // Disambiguate Win10/Win11 via the build number in the kernel version string
                    const QStringList parts = QSysInfo::kernelVersion().split(QLatin1Char('.'));
                    const int build = parts.value(2).toInt();
                    return build >= 22000 ? steam::lang::EOSType::Win11 : steam::lang::EOSType::Windows10;
                }
                default: return steam::lang::EOSType::WinUnknown;
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
                    case 7:  return steam::lang::EOSType::MacOS107;  // Lion
                    case 8:  return steam::lang::EOSType::MacOS108;  // Mountain Lion
                    case 9:  return steam::lang::EOSType::MacOS109;  // Mavericks
                    case 10: return steam::lang::EOSType::MacOS1010; // Yosemite
                    case 11: return steam::lang::EOSType::MacOS1011; // El Capitan
                    case 12: return steam::lang::EOSType::MacOS1012; // Sierra
                    case 13: return steam::lang::EOSType::Macos1013; // High Sierra
                    case 14: return steam::lang::EOSType::Macos1014; // Mojave
                    case 15: return steam::lang::EOSType::Macos1015; // Catalina
                    default: return steam::lang::EOSType::MacOSUnknown;
                }
            }
            switch (major)
            {
                case 11: return steam::lang::EOSType::MacOS11; // Big Sur
                case 12: return steam::lang::EOSType::MacOS12; // Monterey
                case 13: return steam::lang::EOSType::MacOS13; // Ventura
                case 14: return steam::lang::EOSType::MacOS14; // Sonoma
                case 15: return steam::lang::EOSType::MacOS15; // Sequoia
                default: return steam::lang::EOSType::MacOSUnknown;
            }
        }

        default: return steam::lang::EOSType::Unknown;
    }
}