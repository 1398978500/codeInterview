project(Test)

set(TEST_SOURCES main.cpp)

add_executable(${PROJECT_NAME}
    ${TEST_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${redis++_SOURCE_DIR}
    /usr/include/mysql
)

target_link_libraries(${PROJECT_NAME}
    redis++  mysqlclient mysqlcppconn pthread
)

