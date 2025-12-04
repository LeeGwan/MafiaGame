cmake_minimum_required(VERSION 3.16)
project(AuthServer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 컴파일러 설정
set(CMAKE_CXX_COMPILER clang++-17)

# 컴파일 옵션
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wcast-align -Wconversion -Wsign-conversion -Wnull-dereference")

# Debug 모드 설정
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
# MySQL 라이브러리 찾기 (pkg-config 없이)
find_path(MYSQL_INCLUDE_DIR 
    NAMES mysql/mysql.h
    PATHS /usr/include /usr/local/include
)

find_library(MYSQL_LIBRARY
    NAMES mysqlclient
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_LIBRARY)
    message(FATAL_ERROR "MySQL not found. Install with: sudo apt-get install libmysqlclient-dev")
endif()
include_directories(
    ${MYSQL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/MemoryPool
    ${CMAKE_SOURCE_DIR}/Network
    ${CMAKE_SOURCE_DIR}/Network/Aes
    ${CMAKE_SOURCE_DIR}/Network/Mafia_DB
    ${CMAKE_SOURCE_DIR}/Network/Packet
    ${CMAKE_SOURCE_DIR}/Network/Packet/CompactBinaryReader
    ${CMAKE_SOURCE_DIR}/Network/Packet/OptimizedBinaryPacketSerializer
    ${CMAKE_SOURCE_DIR}/Network/Packet/PacketStructure
    ${CMAKE_SOURCE_DIR}/Network/Packet/RoutineProgress
    ${CMAKE_SOURCE_DIR}/Network/Packet/XXHash32
    ${CMAKE_SOURCE_DIR}/Network/ServerConnector
    ${CMAKE_SOURCE_DIR}/Network/ServerConnector/RoutingServerConnector
    ${CMAKE_SOURCE_DIR}/Network/ServerConnector/SessionServerConnector
)

# 소스 파일들
set(SOURCES
    main.cpp
    utils/UtilsForString.cpp
    MemoryPool/MemoryPool.cpp
    Network/Network.cpp
    Network/Aes/Aes.cpp
    Network/Mafia_DB/MafiaDatabase.cpp
    Network/Mafia_DB/ConnectionPool.cpp
    Network/Packet/RoutineProgress/RoutineProgress.cpp
    Network/Packet/CompactBinaryReader/CompactBinaryReader.cpp
    Network/Packet/OptimizedBinaryPacketSerializer/OptimizedBinaryPacketSerializer.cpp
    Network/Packet/XXHash32/XXHash32.cpp
    Network/ServerConnector/RoutingServerConnector/RoutingServerConnector.cpp
    Network/ServerConnector/SessionServerConnector/SessionServerConnector.cpp
)

# 실행 파일 생성
add_executable(AuthServer ${SOURCES})

# 라이브러리 링크
target_link_libraries(AuthServer ssl crypto ${MYSQL_LIBRARY}) 
