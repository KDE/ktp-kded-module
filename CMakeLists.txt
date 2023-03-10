cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "23")
set(RELEASE_SERVICE_VERSION_MINOR "07")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(KTP_KDED_MODULE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(ktp-kded-integration-module VERSION ${KTP_KDED_MODULE_VERSION})

find_package(ECM 1.6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package (Qt5 REQUIRED COMPONENTS Concurrent Widgets Sql Network Test)
find_package (KF5 REQUIRED COMPONENTS DBusAddons WidgetsAddons Config I18n KIO Notifications ConfigWidgets IdleTime KCMUtils Activities)

find_package (KTp REQUIRED)

# Add an option for building tests
option(ENABLE_TESTS "Enable compilation of tests" FALSE)

add_definitions (-DQT_NO_CAST_FROM_ASCII
                 -DQT_NO_KEYWORDS
                 -DTRANSLATION_DOMAIN=\"kded_ktp_integration_module\"
)

include_directories (${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
)

set (kded_ktp_integration_module_SRCS
     ktp_kded_debug.cpp
     contact-cache.cpp
     contact-request-handler.cpp
     telepathy-kded-module-plugin.cpp
     telepathy-module.cpp
     account-status-helper.cpp
     autoaway.cpp
     telepathy-mpris.cpp
     status-handler.cpp
     error-handler.cpp
     contactnotify.cpp
     screensaveraway.cpp
     status-message-parser.cpp
)

add_library (kded_ktp_integration_module MODULE ${kded_ktp_integration_module_SRCS})
set_target_properties (kded_ktp_integration_module PROPERTIES OUTPUT_NAME ktp_integration_module)
kcoreaddons_desktop_to_json(kded_ktp_integration_module ktp_integration_module.desktop)

target_link_libraries (kded_ktp_integration_module
                       KTp::CommonInternals
                       KTp::Widgets
                       KF5::DBusAddons
                       KF5::ConfigGui
                       KF5::ConfigCore
                       KF5::I18n
                       KF5::Notifications
                       KF5::IdleTime
                       KF5::KIOWidgets
                       KF5::WidgetsAddons
                       KF5::Activities
                       ${TELEPATHY_QT5_LIBRARIES}
                       Qt5::Concurrent
                       Qt5::Sql
                       Qt5::Network
)

install (TARGETS kded_ktp_integration_module
         DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded
)

find_file(DBUS_SEND_PATH dbus-send HINTS /usr/bin)
configure_file(org.freedesktop.Telepathy.Client.KTp.KdedIntegrationModule.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.KdedIntegrationModule.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.KdedIntegrationModule.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})

add_subdirectory(config)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()

ki18n_install(po)
