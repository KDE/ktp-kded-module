project(ktp-kded-integration-module)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

# KDE Application Version, managed by release script
set(KDE_APPLICATIONS_VERSION_MAJOR "20")
set(KDE_APPLICATIONS_VERSION_MINOR "03")
set(KDE_APPLICATIONS_VERSION_MICRO "70")
set(KTP_KDED_MODULE_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")

cmake_policy(SET CMP0002 OLD)

set(IS_KTP_INTERNAL_MODULE TRUE)

find_package(ECM 1.3.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH})

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

add_library (kded_ktp_integration_module MODULE ${kded_ktp_integration_module_SRCS}
)

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
         DESTINATION ${PLUGIN_INSTALL_DIR}
)

install (FILES ktp_integration_module.desktop
         DESTINATION ${SERVICES_INSTALL_DIR}/kded
)

find_file(DBUS_SEND_PATH dbus-send HINTS /usr/bin)
configure_file(org.freedesktop.Telepathy.Client.KTp.KdedIntegrationModule.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.KdedIntegrationModule.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.KdedIntegrationModule.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})

add_subdirectory(config)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
