remove_definitions (-DQT_NO_CAST_FROM_ASCII)

set(contactcache_test_SRCS
    ../contact-cache.cpp
    ../ktp_kded_debug.cpp
    contact-cache-main.cpp
)

add_executable(contact-cache
    ${contactcache_test_SRCS}
)

target_link_libraries(contact-cache
  KTp::CommonInternals
  KTp::Widgets
  Qt5::Sql
  KF5::ConfigCore
  ${TELEPATHY_QT5_LIBRARIES}
)

set(status_test_SRCS
    ../status-handler.cpp
    ../ktp_kded_debug.cpp
    ../telepathy-kded-module-plugin.cpp
    ../telepathy-mpris.cpp
    ../status-message-parser.cpp
    ../screensaveraway.cpp
    ../autoaway.cpp
    ../account-status-helper.cpp
    status-handling-main.cpp
)

add_executable(status-test
    ${status_test_SRCS}
)

target_link_libraries(status-test
  KTp::CommonInternals
  KTp::Widgets
  KF5::Activities
  KF5::ConfigCore
  ${TELEPATHY_QT5_LIBRARIES}
  KF5::IdleTime
)

set(parser_test_SRCS
    ../status-message-parser.cpp
    ../telepathy-mpris.cpp
    ../ktp_kded_debug.cpp
    status-message-parser.cpp
)

add_executable(parser-test
    ${parser_test_SRCS}
)

target_link_libraries(parser-test
  KTp::CommonInternals
  KTp::Widgets
  Qt5::Test
)
