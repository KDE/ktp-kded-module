set(telepathy_kded_module_config_SRCS
        telepathy-kded-config.cpp
)

set(telepathy_kded_module_config_UI
    telepathy-kded-config.ui
)

kde4_add_ui_files(telepathy_kded_module_config_SRCS ${telepathy_kded_module_config_UI})

kde4_add_plugin(kcm_telepathy_kded_module_config
                    ${telepathy_kded_module_config_SRCS})

target_link_libraries(kcm_telepathy_kded_module_config ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KCMUTILS_LIBS} )


install(TARGETS kcm_telepathy_kded_module_config
	  DESTINATION ${PLUGIN_INSTALL_DIR})

install (FILES kcm_telepathy_kded_module_config.desktop
         DESTINATION ${SERVICES_INSTALL_DIR}
)
