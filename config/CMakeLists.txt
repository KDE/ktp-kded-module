set(kcm_ktp_integration_module_SRCS
        column-resizer.cpp
        telepathy-kded-config.cpp
        nowplaying-lineedit.cpp
        nowplaying-listwidget.cpp
)

set(kcm_ktp_integration_module_UI
    telepathy-kded-config.ui
)

kde4_add_ui_files(kcm_ktp_integration_module_SRCS ${kcm_ktp_integration_module_UI})

kde4_add_plugin(kcm_ktp_integration_module
                    ${kcm_ktp_integration_module_SRCS})

target_link_libraries(kcm_ktp_integration_module ${KDE4_KDECORE_LIBS}
                                                       ${KDE4_KDEUI_LIBS}
                                                       ${KDE4_KCMUTILS_LIBS}
                                                       ${KDE4_KIO_LIBS}
)


install(TARGETS kcm_ktp_integration_module
        DESTINATION ${PLUGIN_INSTALL_DIR})

install (FILES kcm_ktp_integration_module.desktop
         DESTINATION ${SERVICES_INSTALL_DIR}
)
