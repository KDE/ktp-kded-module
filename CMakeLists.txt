project(ktp-kded-integration-module)

set(KTP_CONTACT_LIST_VERSION "0.8.80")

cmake_policy(SET CMP0002 OLD)

set(IS_KTP_INTERNAL_MODULE TRUE)

find_package(ECM 1.3.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)

find_package (Qt5 REQUIRED COMPONENTS Concurrent Widgets Sql)
find_package (KF5 REQUIRED COMPONENTS DBusAddons Config I18n Notifications ConfigWidgets IdleTime KDELibs4Support)

find_package (KTp REQUIRED)

# Add an option for building tests
option(ENABLE_TESTS "Enable compilation of tests" FALSE)

# make some more macros available
include (MacroLibrary)

add_definitions (-DQT_NO_CAST_FROM_ASCII
                 -DQT_NO_KEYWORDS
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
     autoaway.cpp
     telepathy-mpris.cpp
     status-handler.cpp
     autoconnect.cpp
     error-handler.cpp
     contactnotify.cpp
     screensaveraway.cpp
)

add_library (kded_ktp_integration_module MODULE ${kded_ktp_integration_module_SRCS}
)

target_link_libraries (kded_ktp_integration_module
                       KF5::DBusAddons
                       KF5::ConfigGui
                       KF5::I18n
                       KF5::Notifications
                       KF5::IdleTime
                       KF5::KDELibs4Support
                       ${TELEPATHY_QT5_LIBRARIES}
                       KTp::CommonInternals
                       Qt5::Concurrent
                       Qt5::Sql
)

install (TARGETS kded_ktp_integration_module
         DESTINATION ${PLUGIN_INSTALL_DIR}
)

install (FILES kded_ktp_integration_module.desktop
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
