/**
 * @file RoutineProgress.cpp
 * @brief Implementation of the asynchronous packet processing engine and thread pool.
 */

#include "RoutineProgress.h"
#include "../../../Core/Core.h"
#include "../../../Event/EventManager/EventManager.h"
#include "../../../Event/EventType/EventType.h"
#include "../../../gui/EUIType/EUIType.h"
#include "../../Aes/Aes.h"
#include "../../Network/NetWork.h"
#include "../CompactBinaryReader/CompactBinaryReader.h"
#include "../OptimizedBinaryPacketSerializer/OptimizedBinaryPacketSerializer.h"
#include "../PacketStructure/PacketStructure.h"
#include "../../../gui/guicontrol/GuiControl.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

/** Global Singleton for Packet Processing Management */
std::unique_ptr<RoutineProgress> G_Routine = std::make_unique<RoutineProgress>(1);

/**
 * @brief Initializes the worker thread pool.
 * @param in_threadcount Number of concurrent worker threads to spawn.
 */
RoutineProgress::RoutineProgress(uint8_t in_threadcount)
    : threadcount(in_threadcount), ProsessThreads_status(true) {
    
    // Pre-allocate thread vector to prevent dynamic reallocation overhead
    ProsessThreads.reserve(threadcount);

    // Launch worker threads for parallel packet processing
    for (int i = 0; i < threadcount; ++i) {
        ProsessThreads.emplace_back(&RoutineProgress::RoutineProgressWorkerThread, this, i);
    }
}

RoutineProgress::~RoutineProgress() { Release(); }

void RoutineProgress::SendData_to_Sendque(const std::vector<uint8_t>& data) {
    G_network->addToSendQueue(data);
}

/**
 * @brief Decrypts and dispatches received network packets.
 * * Handles the full lifecycle of a received packet:
 * 1. AES Decryption
 * 2. Binary Deserialization
 * 3. Type-based Event Dispatching
 */
void RoutineProgress::HandleReceivedPacket(const std::vector<uint8_t>& data) {
    try {
        if (!AES) return;

        std::vector<uint8_t> decrypted_data;
        AES->Aes_Decrypt(&data, &decrypted_data);

        PacketType packet_type;
        // RAII management for the binary reader to ensure memory safety
        auto reader = std::unique_ptr<CompactBinaryReader>(new CompactBinaryReader());

        // Parse secure packet header to identify the packet type
        if (!OptimizedBinaryPacketSerializer::ParseSecurePacket(decrypted_data, packet_type, reader.get())) {
            return;
        }

        /**
         * @section Packet_Dispatcher
         * Routes packets to specific game/system events based on the identified PacketType.
         */
        switch (packet_type) {
        case PacketType::FindAccountServerResponse: {
            ServerInfoPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<ServerInfoPacket>(*reader, packet);
            G_core->get_C_eventmanager()->trigger(EventType::SUCESS_ROUTINEAUTH, false, packet.IP, packet.port);
            break;
        }

        case PacketType::RegisterResponse: {
            ResultPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<ResultPacket>(*reader, packet);
            if (packet.ResultTypes == ResultType::SignUp_Succeeded) {
                G_core->get_C_eventmanager()->trigger(EventType::CHANGE_UI_TYPE, false, EUIType::Init);
            }
            break;
        }

        case PacketType::LoginResponse: {
            ResultAndHashPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<ResultAndHashPacket>(*reader, packet);
            if (packet.ResultTypes == ResultType::Login_Succeeded) {
                G_GuiControl->hash = packet.hash;
                // Initialize security layer (Anti-Cheat) synchronously upon login
                G_core->get_C_eventmanager()->trigger(EventType::SECURITY_Init_EVENT, true, packet.hash);
                G_core->get_C_eventmanager()->trigger(EventType::SUCESS_GAMELOBBY, false);
            }
            break;
        }

        case PacketType::TryConnectLobbyServerResponse: {
            ResultAndHashPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<ResultAndHashPacket>(*reader, packet);
            if (packet.ResultTypes == ResultType::CheckSession_Succeeded) {
                G_core->get_C_eventmanager()->trigger(EventType::CHANGE_UI_TYPE, false, EUIType::Lobby);
            }
            break;
        }

        case PacketType::JoinRoomResponse: {
            ResultPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<ResultPacket>(*reader, packet);
            if (packet.ResultTypes == ResultType::JoinRoom_Succeeded) {
                G_core->get_C_eventmanager()->trigger(EventType::CHANGE_UI_TYPE, false, EUIType::Matching);
            }
            break;
        }

        case PacketType::HeartbeatRequest: {
            // Validate Anti-Cheat driver state and send verification response
            G_core->get_C_eventmanager()->trigger(EventType::SECURITY_Heartbeat_EVENT, false, PacketType::HeartbeatResponse);
            break;
        }

        case PacketType::GameCreate: {
            ServerInfoPacket info;
            OptimizedBinaryPacketSerializer::DeserializePacket<ServerInfoPacket>(*reader, info);
            if (!info.IP.empty()) {
                G_core->get_C_eventmanager()->trigger(EventType::CHANGE_UI_TYPE, true, EUIType::Game);
                G_core->get_C_eventmanager()->trigger(EventType::STARTGAME_EVENT, false, info.IP, info.port);
            }
            break;
        }
        default: break;
        }
    }
    catch (...) {
        return; // Silent failure in production to maintain stability
    }
}

/** @brief Enqueues raw data for background processing and signals a worker thread. */
void RoutineProgress::addToProgressQueue(const std::vector<uint8_t>& data) {
    {
        std::lock_guard<std::mutex> lock(routine_queue_mutex);
        data_queue.push(data);
    }
    wakeUpthread.notify_one();
}

/** @brief Shuts down the thread pool and ensures all worker threads join. */
void RoutineProgress::Release() {
    ProsessThreads_status.store(false);
    wakeUpthread.notify_all();

    for (auto& thread : ProsessThreads) {
        if (thread.joinable()) thread.join();
    }
}

/**
 * @brief Main execution loop for worker threads.
 * Uses condition variables for efficient idle/wake cycles.
 */
void RoutineProgress::RoutineProgressWorkerThread(int threadId) {
    while (ProsessThreads_status.load()) {
        std::vector<uint8_t> data;
        {
            std::unique_lock<std::mutex> lock(routine_queue_mutex);
            wakeUpthread.wait(lock, [this]() {
                return !data_queue.empty() || !ProsessThreads_status.load();
            });

            if (!ProsessThreads_status.load()) break;

            data = data_queue.front();
            data_queue.pop();
        }
        HandleReceivedPacket(data);
    }
}

// --- Specialized Packet Serialization & Transmission ---

void RoutineProgress::SendResponseForTypePacket(PacketType type) {
    TypePacket packet;
    packet.Type = type;
    SerializeAndSendResponse<TypePacket>(packet);
}

void RoutineProgress::SendResponseForTwoStringPacket(PacketType type, const std::string& str1, const std::string& str2) {
    TwoStringPacket packet;
    packet.Type = type;
    packet.str1 = str1;
    packet.str2 = str2;
    SerializeAndSendResponse<TwoStringPacket>(packet);
}

void RoutineProgress::SendResponseForHashPacket(PacketType type, const std::string& str1) {
    HashPacket packet;
    packet.Type = type;
    packet.hash = str1;
    SerializeAndSendResponse<HashPacket>(packet);
}

/**
 * @brief Sends high-priority packets that bypass the standard send queue.
 * Performs immediate encryption and returns the raw ciphertext.
 */
std::vector<uint8_t> RoutineProgress::SendResponseForpriorityPacket(PacketType type, const std::string& str1) {
    HashPacket packet;
    packet.Type = type;
    packet.hash = str1;
    
    std::vector<uint8_t> data;
    std::vector<uint8_t> encrypted_data;

    OptimizedBinaryPacketSerializer::SerializePacket<HashPacket>(packet, &data);
    AES->Aes_Encrypt(&data, &encrypted_data);

    return encrypted_data;
}

/**
 * @brief Generic pipeline for packet egress.
 * Performs serialization, AES encryption, and enqueues to the network layer.
 */
template<typename T>
void RoutineProgress::SerializeAndSendResponse(const T& response_packet) {
    std::vector<uint8_t> data;
    std::vector<uint8_t> encrypted_data;

    OptimizedBinaryPacketSerializer::SerializePacket<T>(response_packet, &data);
    AES->Aes_Encrypt(&data, &encrypted_data);
    SendData_to_Sendque(encrypted_data);
}
