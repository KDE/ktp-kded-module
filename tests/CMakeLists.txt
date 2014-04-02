remove_definitions (-DQT_NO_CAST_FROM_ASCII)

set(contactcache_test_SRCS
    ../contact-cache.cpp
    contact-cache-main.cpp
)

kde4_add_executable(contact-cache
    ${contactcache_test_SRCS}
)

target_link_libraries(contact-cache
  ${QT_QTSQL_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${TELEPATHY_QT4_LIBRARIES}
  ${KTP_LIBRARIES}
)

set(status_test_SRCS
    ../status-handler.cpp
    ../telepathy-kded-module-plugin.cpp
    ../telepathy-mpris.cpp
    ../screensaveraway.cpp
    ../autoaway.cpp
    ../autoconnect.cpp
    status-handling-main.cpp
)

kde4_add_executable(status-test
    ${status_test_SRCS}
)

target_link_libraries(status-test
  ${KDE4_KDECORE_LIBS}
  ${TELEPATHY_QT4_LIBRARIES}
  ${KTP_LIBRARIES}
  ${KDE4_KIDLETIME_LIBRARY}
)

