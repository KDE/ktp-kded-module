
set(contactcache_test_SRCS
    ../contact-cache.cpp
    contact-cache-main.cpp
)

###

kde4_add_executable(contact-cache
    ${contactcache_test_SRCS}
)

target_link_libraries(contact-cache
  ${QT_QTSQL_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${TELEPATHY_QT4_LIBRARIES}
  ${KTP_LIBRARIES}
)
