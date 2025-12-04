cmake_minimum_required(VERSION 3.16)
project(RoutingServerForAuth)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER clang++-17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wcast-align -Wconversion -Wsign-conversion -Wnull-dereference")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/MemoryPool
    ${CMAKE_SOURCE_DIR}/Network
    ${CMAKE_SOURCE_DIR}/Network/Aes
    ${CMAKE_SOURCE_DIR}/Network/Packet
    ${CMAKE_SOURCE_DIR}/Network/Packet/CompactBinaryReader
    ${CMAKE_SOURCE_DIR}/Network/Packet/OptimizedBinaryPacketSerializer
    ${CMAKE_SOURCE_DIR}/Network/Packet/PacketStructure
    ${CMAKE_SOURCE_DIR}/Network/Packet/RoutineProgress
    ${CMAKE_SOURCE_DIR}/Network/Packet/XXHash32
    ${CMAKE_SOURCE_DIR}/Network/ServerInfo
    ${CMAKE_SOURCE_DIR}/Network/ServerRegistry
)
set(SOURCES
    main.cpp
    utils/UtilsForString.cpp
    MemoryPool/MemoryPool.cpp
    Network/Network.cpp
    Network/Packet/RoutineProgress/RoutineProgress.cpp
    Network/Aes/Aes.cpp
    Network/Packet/CompactBinaryReader/CompactBinaryReader.cpp
    Network/Packet/OptimizedBinaryPacketSerializer/OptimizedBinaryPacketSerializer.cpp
    Network/Packet/XXHash32/XXHash32.cpp
    Network/ServerInfo/ServerInfo.cpp
    Network/ServerRegistry/ServerRegistry.cpp
)
add_executable(RoutingServerForAuth ${SOURCES})
target_link_libraries(RoutingServerForAuth ssl crypto)
