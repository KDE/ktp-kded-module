project(telepathy-kded-module)

set (CMAKE_MODULE_PATH
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
     ${CMAKE_MODULE_PATH}
)

find_package (KDE4 REQUIRED)
find_package (TelepathyQt4 0.7.1 REQUIRED)

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
)

set (telepathy_module_SRCS
     telepathy-kded-module-plugin.cpp
     global-presence.cpp
     telepathy-module.cpp
     autoaway.cpp
     telepathy-mpris.cpp
     error-handler.cpp
)

kde4_add_plugin (kded_telepathy_module
                 ${telepathy_module_SRCS}
)

target_link_libraries (kded_telepathy_module
                       ${KDE4_KDECORE_LIBS}
                       ${TELEPATHY_QT4_LIBRARIES}
                       ${KDE4_KDEUI_LIBS}
                       kidletime

)

install (TARGETS kded_telepathy_module
         DESTINATION ${PLUGIN_INSTALL_DIR}
)

install (FILES telepathy_kded_module.desktop
         DESTINATION ${SERVICES_INSTALL_DIR}/kded
)


add_subdirectory(config)
