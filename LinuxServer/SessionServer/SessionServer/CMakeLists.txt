cmake_minimum_required(VERSION 3.16)
project(Sessionserver)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 컴파일러 설정
set(CMAKE_CXX_COMPILER clang++-17)

# 컴파일 옵션
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wcast-align -Wconversion -Wsign-conversion -Wnull-dereference")

# Debug 모드 설정
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")

include_directories(

    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/MemoryPool
    ${CMAKE_SOURCE_DIR}/Session
    ${CMAKE_SOURCE_DIR}/Session/ClientInfo
    ${CMAKE_SOURCE_DIR}/Session/SessionManager
    ${CMAKE_SOURCE_DIR}/Network
    ${CMAKE_SOURCE_DIR}/Network/Aes
    ${CMAKE_SOURCE_DIR}/Network/Packet
    ${CMAKE_SOURCE_DIR}/Network/Packet/CompactBinaryReader
    ${CMAKE_SOURCE_DIR}/Network/Packet/OptimizedBinaryPacketSerializer
    ${CMAKE_SOURCE_DIR}/Network/Packet/PacketStructure
    ${CMAKE_SOURCE_DIR}/Network/Packet/RoutineProgress
    ${CMAKE_SOURCE_DIR}/Network/Packet/XXHash32

)

# 소스 파일들
set(SOURCES
    main.cpp
    utils/UtilsForString.cpp
    MemoryPool/MemoryPool.cpp
    Session/ClientInfo/ClientInfo.cpp
    Session/SessionManager/SessionManager.cpp
    Network/Network.cpp
    Network/Packet/RoutineProgress/RoutineProgress.cpp
    Network/Aes/Aes.cpp
    Network/Packet/CompactBinaryReader/CompactBinaryReader.cpp
    Network/Packet/OptimizedBinaryPacketSerializer/OptimizedBinaryPacketSerializer.cpp
    Network/Packet/XXHash32/XXHash32.cpp

)

# 실행 파일 생성
add_executable(Sessionserver ${SOURCES})

# 라이브러리 링크
target_link_libraries(Sessionserver ssl crypto) 