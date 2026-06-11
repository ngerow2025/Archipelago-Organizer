cmake_minimum_required(VERSION 3.16) 

project(Archipelago-Organiser VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)
find_package(Protobuf CONFIG REQUIRED)

add_library(proto-objects OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/clientmetrics.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/content_manifest.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/contenthubs.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/encrypted_app_ticket.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/enums.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/enums_clientserver.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/enums_productinfo.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/htmlmessages.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/offline_ticket.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steamdatagram_messages_auth.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steamdatagram_messages_sdr.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_accounthardware.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_appoverview.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_auth.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_base.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_broadcast.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_chat.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_childprocessquery.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_client_objects.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientlanp2p.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientmetrics.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientnotificationtypes.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver_2.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver_appinfo.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver_friends.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver_gameservers.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver_lbs.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver_login.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver_mms.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver_ucm.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver_uds.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver_userstats.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientserver_video.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_clientsettings.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_cloud.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_community.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_contentsystem.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_credentials.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_datapublisher.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_depotbuilder.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_econ.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_familygroups.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_friendmessages.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_gamenetworking.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_gamenetworkingui.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_gamenotifications.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_gamerecording.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_gamerecording_objects.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_gameservers.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_hiddevices.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_inventory.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_linkfilter.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_lobbymatchmaking.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_market.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_marketingmessages.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_notifications.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_offline.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_parental.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_parental_objects.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_parties.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_partnerapps.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_player.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_publishedfile.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_qms.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_remoteclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_remoteclient_discovery.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_remoteclient_service.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_remoteclient_service_messages.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_remoteplay.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_secrets.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_shader.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_site_license.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_sitelicenseclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_siteserverui.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_steamtv.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_store.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_storebrowse.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_timedtrial.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_twofactor.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_unified_base.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_unified_test.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_useraccount.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_vac.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_video.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_virtualcontroller.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steammessages_workshop.steamclient.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steamnetworkingsockets_messages.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steamnetworkingsockets_messages_certs.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/steamnetworkingsockets_messages_udp.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_achievements.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_audio.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_base.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_bluetooth.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_gamenotes.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_gamerecording.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_gamerecordingfiles.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_gamescope.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_hardwareupdate.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_leds.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_screensaver.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_sharedjscontext.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_sleep.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_steamengine.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_steaminput.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_steamos.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_storagedevicemanager.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_systemmanager.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_transport.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_transportvalidation.proto"
    "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam/webuimessages_user.proto"
)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_LIST_DIR}/src/steam-protoc-gen") 
set(PROTO_IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}/Steam-Protobufs/steam")

target_include_directories(proto-objects PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

protobuf_generate(
    TARGET proto-objects
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")


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

target_link_libraries(Archipelago-Organiser PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(Archipelago-Organiser PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(Archipelago-Organiser PRIVATE cap)
target_link_libraries(Archipelago-Organiser PRIVATE proto-objects)


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
