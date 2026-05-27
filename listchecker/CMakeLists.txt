# last chaged 26.5.2026

cmake_minimum_required(VERSION 3.16)
project(listchecker)
add_definitions(-DLISTCHECKER)

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

find_package(Boost 1.83 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

## curl
find_package(CURL 8.5.0)
if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    message(STATUS "Curl 8.5.0 found")
    add_definitions(-DHAVE_CURL)
endif()

FILE(GLOB SOURCES
    ${PROJECT_SOURCE_DIR}/main.cxx
    ${PROJECT_SOURCE_DIR}/listchecker.hpp
    ${PROJECT_SOURCE_DIR}/listchecker.cxx
    ${PROJECT_SOURCE_DIR}/clientdbus.cxx
    ${PROJECT_SOURCE_DIR}/clientdbus.hpp
    ${PROJECT_SOURCE_DIR}/../src/abstract_chanserv.cpp
    ${PROJECT_SOURCE_DIR}/../src/abstract_chanserv.h
    ${PROJECT_SOURCE_DIR}/../src/adaptorimp.cpp
    ${PROJECT_SOURCE_DIR}/../src/adaptorimp.h
    ${PROJECT_SOURCE_DIR}/../src/jiraapi.cpp
    ${PROJECT_SOURCE_DIR}/../src/jiraapi.h
    ${PROJECT_SOURCE_DIR}/../src/simpleini.h
    ${PROJECT_SOURCE_DIR}/../src/structures.h
    ${PROJECT_SOURCE_DIR}/../src/support.cpp
    ${PROJECT_SOURCE_DIR}/../src/support.h
    ${PROJECT_SOURCE_DIR}/../src/trelloapi.cpp
    ${PROJECT_SOURCE_DIR}/../src/trelloapi.h
    ${PROJECT_SOURCE_DIR}/../src/chanconf.cpp
    ${PROJECT_SOURCE_DIR}/../src/chanconf.h
)

add_executable(listchecker ${SOURCES})

# find_package(cpprestsdk REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Network DBus Sql HttpServer)
# find_package(manapihttp)

target_link_libraries(listchecker m manapihttp
    ${PROJECT_SOURCE_DIR}/../lib/libTgBot.a # Должно идти ДО ssl и crypto
    # Иначе будет "Boost: undefined reference to symbol 'SSL_get_verify_callback@@OPENSSL_3.0.0'
    ssl crypto # cpprestsdk::cpprest cjson
    odbc pthread
    Qt6::Sql
    Qt6::Core
    Qt6::DBus
    Qt6::Network
    Qt6::HttpServer
    ${CURL_LIBRARIES})

target_include_directories(
    listchecker PUBLIC
    ${PROJECT_SOURCE_DIR}
    # ${PROJECT_SOURCE_DIR}/../lib/cpprestsdk
    ${PROJECT_SOURCE_DIR}/../lib/tgbot/include
)

add_custom_target(copy_dependencies ALL
    COMMENT "Copying dependencies to ${PROJECT_SOURCE_DIR}/bin"
)

set(
    DEPENDENCIES
    ${ICUUC_LIB} ${ICUI18N_LIB} ${ICUDATA_LIB}
    ${OPENSSL_LIB} ${CRYPTO_LIB}
    ${ZLIB_LIB}
)

foreach(DEPENDENCY ${DEPENDENCIES})
    add_custom_command(
        TARGET copy_dependencies
        COMMAND cp -a -u ${DEPENDENCY}* ${PROJECT_SOURCE_DIR}/bin
        COMMENT "Copying ${DEPENDENCY}* to ${PROJECT_SOURCE_DIR}/bin"
    )
endforeach()


