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

set(CMAKE_BUILD_TYPE Debug)

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
# find_package(manapihttp)
find_package(Boost 1.83 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

#libTgBot:      /* uncomment next line and comment to rebuild new verison of TgBot library */
# include(tgbotsrc.inc)

FILE(GLOB CHANSERV_SRC
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/chanserv_keyboards.cpp
    ${PROJECT_SOURCE_DIR}/src/chanserv_on_any_message.cpp
    ${PROJECT_SOURCE_DIR}/src/chanserv_callbacks.cpp
    ${PROJECT_SOURCE_DIR}/src/chanserv_commands.cpp
    ${PROJECT_SOURCE_DIR}/src/abstract_chanserv.cpp
    ${PROJECT_SOURCE_DIR}/src/abstract_chanserv.h
    ${PROJECT_SOURCE_DIR}/src/chanconf.cpp
    ${PROJECT_SOURCE_DIR}/src/chanconf.h
    ${PROJECT_SOURCE_DIR}/src/adaptorimp.cpp
    ${PROJECT_SOURCE_DIR}/src/adaptorimp.h
    ${PROJECT_SOURCE_DIR}/src/chanserv.cpp
    ${PROJECT_SOURCE_DIR}/src/chanserv.h
    ${PROJECT_SOURCE_DIR}/src/jiraapi.cpp
    ${PROJECT_SOURCE_DIR}/src/jiraapi.h
    ${PROJECT_SOURCE_DIR}/src/serverdbus.cpp
    ${PROJECT_SOURCE_DIR}/src/serverdbus.h
    ${PROJECT_SOURCE_DIR}/src/simpleini.h
    ${PROJECT_SOURCE_DIR}/src/structures.h
    ${PROJECT_SOURCE_DIR}/src/support.cpp
    ${PROJECT_SOURCE_DIR}/src/support.h
    ${PROJECT_SOURCE_DIR}/src/trelloapi.cpp
    ${PROJECT_SOURCE_DIR}/src/trelloapi.h
    ${PROJECT_SOURCE_DIR}/src/sensitive_data.h
#   ${PROJECT_SOURCE_DIR}/src/qtests.cpp
#   ${PROJECT_SOURCE_DIR}/src/qtests.h
#   ${PROJECT_SOURCE_DIR}/src/tm_bot.hpp
#   ${PROJECT_SOURCE_DIR}/src/maxbotapi.h
#   ${PROJECT_SOURCE_DIR}/src/maxbotapi.cpp
)
add_executable(chanserv ${CHANSERV_SRC})
add_test(chanserv TestMinimal)

# add_subdirectory(lib/QXlsx)
target_link_libraries(chanserv m # manapihttp #QXlsx::QXlsx
    #TgBot  /* uncomment and comment to rebuild new verison of TgBot library */    
    ${PROJECT_SOURCE_DIR}/lib/libTgBot.a    # Должно идти ДО ssl и crypto
    boost_context
    boost_coroutine # apt install libboost-coroutine-dev libboost-coroutine1.83.0 libboost-coroutine1.83-dev libcurl4-gnutls-dev unixodbc-dev
    ssl crypto
    odbc pthread
    Qt6::Sql
    Qt6::Test
    Qt6::Core
    Qt6::DBus
    Qt6::Network
    Qt6::HttpServer
    ${CURL_LIBRARIES}
)

target_include_directories(chanserv PUBLIC
    ${PROJECT_SOURCE_DIR}
#   ${PROJECT_SOURCE_DIR}/lib/tgbot/lib/openssl-3.0.0/include
#   ${PROJECT_SOURCE_DIR}/lib/tgbot/lib/boost_1_65_1            /* we'll use system's libboost-dev package */
    ${PROJECT_SOURCE_DIR}/lib/tgbot/include
)

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
