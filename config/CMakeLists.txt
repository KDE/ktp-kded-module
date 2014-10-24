include_directories (${TELEPATHY_QT5_INCLUDE_DIR})

set(kcm_ktp_integration_module_SRCS
        column-resizer.cpp
        telepathy-kded-config.cpp
        nowplaying-lineedit.cpp
        nowplaying-listwidget.cpp
)

set(kcm_ktp_integration_module_UI
    telepathy-kded-config.ui
)

ki18n_wrap_ui(kcm_ktp_integration_module_SRCS ${kcm_ktp_integration_module_UI})

add_library(kcm_ktp_integration_module MODULE ${kcm_ktp_integration_module_SRCS})

target_link_libraries(kcm_ktp_integration_module Qt5::Widgets KF5::I18n KF5::ConfigWidgets KF5::KDELibs4Support)


install(TARGETS kcm_ktp_integration_module
        DESTINATION ${PLUGIN_INSTALL_DIR})

install (FILES kcm_ktp_integration_module.desktop
         DESTINATION ${SERVICES_INSTALL_DIR}
)
