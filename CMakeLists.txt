project(ktp-kded-integration-module)

set(KTP_CONTACT_LIST_VERSION "0.6.80")

set (CMAKE_MODULE_PATH
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
     ${CMAKE_MODULE_PATH}
)
cmake_policy(SET CMP0002 OLD)

set(IS_KTP_INTERNAL_MODULE TRUE)
find_package (KDE4 REQUIRED)
find_package (TelepathyQt4 0.8.9 REQUIRED)
find_package (KTp REQUIRED)

# set some default settings
include (KDE4Defaults)

# make some more macros available
include (MacroLibrary)

add_definitions (${KDE4_DEFINITIONS}
                 -DQT_NO_CAST_FROM_ASCII
                 -DQT_NO_KEYWORDS
)

include_directories (${KDE4_INCLUDES}
                     ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${TELEPATHY_QT4_INCLUDE_DIR}
                     ${KTP_INCLUDE_DIR}
)

set (kded_ktp_integration_module_SRCS
     contact-request-handler.cpp
     telepathy-kded-module-plugin.cpp
     telepathy-module.cpp
     autoaway.cpp
     telepathy-mpris.cpp
     autoconnect.cpp
     error-handler.cpp
     contactnotify.cpp
     screensaveraway.cpp
)

kde4_add_plugin (kded_ktp_integration_module
                 ${kded_ktp_integration_module_SRCS}
)

target_link_libraries (kded_ktp_integration_module
                       ${KDE4_KDECORE_LIBS}
                       ${TELEPATHY_QT4_LIBRARIES}
                       ${KTP_LIBRARIES}
                       ${KDE4_KDEUI_LIBS}
                       ${KDE4_KIDLETIME_LIBRARY}
                       ${KDE4_SOLID_LIBS}
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
