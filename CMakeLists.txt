cmake_minimum_required(VERSION 3.16) 

project(Archipelago-Organiser VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network WebSockets)
find_package(Protobuf CONFIG REQUIRED)

add_library(proto-objects OBJECT
    "3rd_party/Steam-Protobufs/steam/clientmetrics.proto"
    "3rd_party/Steam-Protobufs/steam/content_manifest.proto"
    "3rd_party/Steam-Protobufs/steam/contenthubs.proto"
    "3rd_party/Steam-Protobufs/steam/encrypted_app_ticket.proto"
    "3rd_party/Steam-Protobufs/steam/enums.proto"
    "3rd_party/Steam-Protobufs/steam/enums_clientserver.proto"
    "3rd_party/Steam-Protobufs/steam/enums_productinfo.proto"
    "3rd_party/Steam-Protobufs/steam/htmlmessages.proto"
    "3rd_party/Steam-Protobufs/steam/offline_ticket.proto"
    "3rd_party/Steam-Protobufs/steam/steamdatagram_messages_auth.proto"
    "3rd_party/Steam-Protobufs/steam/steamdatagram_messages_sdr.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_accounthardware.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_appoverview.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_auth.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_base.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_broadcast.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_chat.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_childprocessquery.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_client_objects.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientlanp2p.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientmetrics.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientnotificationtypes.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver_2.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver_appinfo.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver_friends.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver_gameservers.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver_lbs.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver_login.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver_mms.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver_ucm.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver_uds.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver_userstats.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientserver_video.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_clientsettings.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_cloud.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_community.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_contentsystem.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_credentials.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_datapublisher.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_depotbuilder.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_econ.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_familygroups.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_friendmessages.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_gamenetworking.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_gamenetworkingui.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_gamenotifications.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_gamerecording.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_gamerecording_objects.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_gameservers.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_hiddevices.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_inventory.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_linkfilter.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_lobbymatchmaking.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_market.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_marketingmessages.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_notifications.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_offline.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_parental.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_parental_objects.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_parties.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_partnerapps.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_player.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_publishedfile.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_qms.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_remoteclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_remoteclient_discovery.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_remoteclient_service.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_remoteclient_service_messages.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_remoteplay.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_secrets.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_shader.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_site_license.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_sitelicenseclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_siteserverui.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_steamtv.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_store.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_storebrowse.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_timedtrial.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_twofactor.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_unified_base.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_unified_test.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_useraccount.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_vac.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_video.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_virtualcontroller.proto"
    "3rd_party/Steam-Protobufs/steam/steammessages_workshop.steamclient.proto"
    "3rd_party/Steam-Protobufs/steam/steamnetworkingsockets_messages.proto"
    "3rd_party/Steam-Protobufs/steam/steamnetworkingsockets_messages_certs.proto"
    "3rd_party/Steam-Protobufs/steam/steamnetworkingsockets_messages_udp.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_achievements.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_audio.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_base.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_bluetooth.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_gamenotes.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_gamerecording.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_gamerecordingfiles.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_gamescope.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_hardwareupdate.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_leds.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_screensaver.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_sharedjscontext.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_sleep.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_steamengine.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_steaminput.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_steamos.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_storagedevicemanager.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_systemmanager.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_transport.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_transportvalidation.proto"
    "3rd_party/Steam-Protobufs/steam/webuimessages_user.proto"
)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_LIST_DIR}/src/gen/protobuf") 
set(PROTO_IMPORT_DIRS "3rd_party/Steam-Protobufs/steam")

target_include_directories(proto-objects PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")
target_link_libraries(proto-objects PUBLIC protobuf::libprotobuf)

protobuf_generate(
    TARGET proto-objects
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")

# --- SteamKit SteamLanguage (.steamd) Enum Generation ---
set(STEAM_LANGUAGE_DIR "3rd_party/SteamKit/Resources/SteamLanguage")
set(STEAM_ENUMS_GEN_DIR "${CMAKE_CURRENT_LIST_DIR}/src/gen/enums")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(GENERATED_ENUM_HEADERS
    "${STEAM_ENUMS_GEN_DIR}/emsg.h"
    "${STEAM_ENUMS_GEN_DIR}/enums.h"
    "${STEAM_ENUMS_GEN_DIR}/eresult.h"
    "${STEAM_ENUMS_GEN_DIR}/netheader.h"
    "${STEAM_ENUMS_GEN_DIR}/gamecoordinator.h"
    "${STEAM_ENUMS_GEN_DIR}/header.h"
    "${STEAM_ENUMS_GEN_DIR}/steammsg.h"
)

add_custom_command(
    OUTPUT ${GENERATED_ENUM_HEADERS}
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_LIST_DIR}/parse_steamd.py"
            -i "${STEAM_LANGUAGE_DIR}"
            -o "${STEAM_ENUMS_GEN_DIR}"
            -n "3rd_party/SteamKit"
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/parse_steamd.py"
            "${STEAM_LANGUAGE_DIR}/emsg.steamd"
            "${STEAM_LANGUAGE_DIR}/enums.steamd"
            "${STEAM_LANGUAGE_DIR}/eresult.steamd"
            "${STEAM_LANGUAGE_DIR}/netheader.steamd"
            "${STEAM_LANGUAGE_DIR}/gamecoordinator.steamd"
            "${STEAM_LANGUAGE_DIR}/header.steamd"
            "${STEAM_LANGUAGE_DIR}/steammsg.steamd"
    COMMENT "Generating C++ enums from SteamKit .steamd files"
    VERBATIM
)

add_custom_target(generate-steam-enums DEPENDS ${GENERATED_ENUM_HEADERS})


set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/frontend/games/gameListWidget.cpp
        src/frontend/games/gameListWidget.h
        src/frontend/games/gametabwidget.cpp
        src/frontend/games/gametabwidget.h
        src/frontend/games/steamlibrarieswidget.cpp
        src/frontend/games/steamlibrarieswidget.h
        src/frontend/games/registeredgameswidget.cpp
        src/frontend/games/registeredgameswidget.h
        src/frontend/games/yamlconfigswidget.cpp
        src/frontend/games/yamlconfigswidget.h
        src/logic/sandbox.cpp
        src/logic/sandbox.h
        src/logic/proton.cpp
        src/logic/proton.h
        src/logic/util.cpp
        src/logic/util.h
        src/logic/mounts.cpp
        src/logic/mounts.h
        src/games/ADOFAI/config.cpp
        src/games/ADOFAI/config.h
        src/datastore/database.cpp
        src/datastore/database.h
        src/datastore/protonDiskGame.cpp
        src/datastore/protonDiskGame.h
        src/datastore/steamGame.cpp
        src/datastore/steamGame.h
        src/steam/connectionmanager.cpp
        src/steam/connectionmanager.h
        src/steam/jobID.h
        src/steam/jobID.cpp
        src/steam/connection.cpp
        src/steam/connection.h
        src/steam/authConnection.cpp
        src/steam/authConnection.h
        src/steam/util.cpp
        src/steam/util.h
) 


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Archipelago-Organiser
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Archipelago-Organiser APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Archipelago-Organiser SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Archipelago-Organiser
            ${PROJECT_SOURCES}
        )
    endif()
endif()

add_subdirectory(3rd_party/ValveFileVDF)

# Disable uneeded options
set(MAGIC_ENUM_OPT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MAGIC_ENUM_OPT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MAGIC_ENUM_OPT_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(3rd_party/magic_enum)

target_link_libraries(Archipelago-Organiser PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(Archipelago-Organiser PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(Archipelago-Organiser PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(Archipelago-Organiser PRIVATE cap)
target_link_libraries(Archipelago-Organiser PRIVATE proto-objects)
target_link_libraries(Archipelago-Organiser PRIVATE ValveFileVDF)
target_link_libraries(Archipelago-Organiser PRIVATE magic_enum::magic_enum)
target_link_libraries(Archipelago-Organiser PRIVATE Qt${QT_VERSION_MAJOR}::WebSockets)

add_dependencies(Archipelago-Organiser generate-steam-enums)
target_include_directories(Archipelago-Organiser PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/gen")
target_include_directories(Archipelago-Organiser PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/gen/enums")


target_compile_definitions(Archipelago-Organiser PRIVATE FEATURE_system_sqlite=OFF)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Archipelago-Organiser)
endif()
set_target_properties(Archipelago-Organiser PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Archipelago-Organiser
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Archipelago-Organiser)
endif()
