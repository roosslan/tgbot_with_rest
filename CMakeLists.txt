# roosslan 27.5.26

cmake_minimum_required(VERSION 3.16)
project(chanserv)

enable_testing()

include(FetchContent)
# FetchContent_Declare(
#    googletest
#    GIT_REPOSITORY https://github.com/google/googletest.git
#    GIT_TAG        703bd9caab50b139428cea1aaff9974ebee5742e # release-1.10.0)
# FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

# Тип сборки по умолчанию, если не задан явно (-DCMAKE_BUILD_TYPE=Release и т.п.)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Тип сборки" FORCE)
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
    CACHE PATH "Output directory for shared libraries")
set(BUILD_TESTING OFF)

set(CMAKE_AUTOMOC ON)   # w/o automoc the Q_OBJECT in will throw undefined reference to vtable error

## curl
find_package(CURL 8.5.0)
if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    message(STATUS "Curl 8.5.0 found")
    add_definitions(-DHAVE_CURL)
endif()

find_package(Qt6 REQUIRED COMPONENTS Test Core Network DBus Sql HttpServer)
find_package(Boost 1.83 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# tgbot-cpp собирается из сабмодуля lib/tgbot (git submodule update --init --recursive)
set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/tgbot EXCLUDE_FROM_ALL)

# Всё, кроме main.cpp, собирается в библиотеку: её используют и бот, и тесты
add_library(chanserv_core STATIC
    src/chanserv_keyboards.cpp
    src/chanserv_on_any_message.cpp
    src/chanserv_callbacks.cpp
    src/chanserv_commands.cpp
    src/abstract_chanserv.cpp
    src/abstract_chanserv.h
    src/access.h
    src/chanconf.cpp
    src/chanconf.h
    src/adaptorimp.cpp
    src/adaptorimp.h
    src/chanserv.cpp
    src/chanserv.h
    src/jiraapi.cpp
    src/jiraapi.h
    src/serverdbus.cpp
    src/serverdbus.h
    src/session_store.h
    src/task_queue.h
    src/structures.h
    src/support.cpp
    src/support.h
    src/trelloapi.cpp
    src/trelloapi.h
)

# add_subdirectory(lib/QXlsx)
target_link_libraries(chanserv_core PUBLIC m #QXlsx::QXlsx
    TgBot    # Должно идти ДО ssl и crypto
    boost_context
    boost_coroutine # apt install libboost-coroutine-dev libboost-coroutine1.83.0 libboost-coroutine1.83-dev libcurl4-gnutls-dev unixodbc-dev
    ssl crypto
    odbc pthread
    Qt6::Sql
    Qt6::Core
    Qt6::DBus
    Qt6::Network
    Qt6::HttpServer
    ${CURL_LIBRARIES}
)

target_include_directories(chanserv_core PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/lib/tgbot/include
    ${PROJECT_SOURCE_DIR}/lib/tomlplusplus/include    # toml++ (header-only), чтение chanserv.toml
)

add_executable(chanserv src/main.cpp)
target_link_libraries(chanserv PRIVATE chanserv_core)

# Тесты: ctest --test-dir <build> --output-on-failure
option(CHANSERV_BUILD_TESTS "Собирать тесты chanserv" ON)
if (CHANSERV_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(chanserv_tests tests/tst_chanserv.cpp)
    target_link_libraries(chanserv_tests PRIVATE chanserv_core Qt6::Test)
    target_compile_definitions(chanserv_tests PRIVATE CHANSERV_SAMPLE_CONFIG="${PROJECT_SOURCE_DIR}/docker-img/chanserv.toml")
    set_target_properties(chanserv_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    add_test(NAME chanserv_tests COMMAND chanserv_tests)
endif()

add_custom_target(copy_dependencies ALL
    COMMENT "Copying dependencies to ${PROJECT_SOURCE_DIR}/bin"
)

foreach(DEPENDENCY ${DEPENDENCIES})
    add_custom_command(
        TARGET copy_dependencies
        COMMAND cp -a -u ${DEPENDENCY}* ${PROJECT_SOURCE_DIR}/bin
        COMMENT "Copying ${DEPENDENCY}* to ${PROJECT_SOURCE_DIR}/bin"
    )
endforeach()

# qt6_generate_dbus_interface( serverdbus.h  chanserv.qdbus.xml   OPTIONS -a )
