// Fill out your copyright notice in the Description page of Project Settings.

#include "RoutineProgress.h"
#include "CompactBinaryReader.h"
#include "OptimizedBinaryPacketSerializer.h"
#include "PacketStructure.h"

/** Global unique pointer for centralized routine management */
TUniquePtr<RoutineProgress> GRoutineProgress = MakeUnique<RoutineProgress>();

RoutineProgress::RoutineProgress()
{
}

RoutineProgress::~RoutineProgress()
{
}

/**
 * @brief Main entry point for processing raw incoming binary packets.
 * Deserializes the secure packet header and dispatches to appropriate logic based on PacketType.
 * @param data The raw byte array received from the network.
 */
void RoutineProgress::HandleReceivedPacket(const TArray<uint8_t>& data)
{
    TUniquePtr<CompactBinaryReader> reader = MakeUnique<CompactBinaryReader>();
    PacketType packet_type;

    // Validate packet integrity and parse header via the Serializer
    if (!OptimizedBinaryPacketSerializer::ParseSecurePacket(data, packet_type, reader.Get()) || !reader.Get())
    {
        return;
    }

    switch (packet_type)
    {
    case PacketType::GameCreate:
        {
            /**
             * Handle Game Instance Creation:
             * Synchronizes the authorized session hash list from the Auth/Routine server.
             */
            FUserAuthData Data;
            OptimizedBinaryPacketSerializer::DeserializePacket<FUserAuthData>(*reader, Data);
            
            if (Data.hash.IsEmpty())
            {
                return;
            }

            Hashes = Data.hash;

            // Log synchronized session hashes for debugging/audit
            int32 Index = 1;
            for (const auto& HashEntry : Hashes)
            {
                UE_LOG(LogTemp, Log, TEXT("[Auth] Received Authorized Session Hash [%d]: %s"), Index, *HashEntry);
                Index++;
            }
        
            break;
        }
    default:
        // Handle undefined or unhandled packet types
        break;
    }

    UE_LOG(LogTemp, Display, TEXT("HandleReceivedPacket: Processing complete."));
}

/** @return Returns the current list of authorized session hashes */
TArray<FString> RoutineProgress::GetHashes() const
{
    return Hashes;
}

/** @return Returns the list of registered player nicknames */
TArray<FString> RoutineProgress::GetNickNames() const
{
    return NickNames;
}

void RoutineProgress::SetHashes(const TArray<FString>& In_Hashs)
{
    Hashes = In_Hashs;
}

void RoutineProgress::SetNickNames(const TArray<FString>& In_NickNames)
{
    NickNames = In_NickNames;
}

/**
 * @brief Validates if a specific session hash is present in the authorized whitelist.
 * Acts as a primary gatekeeper for player login requests.
 * @param In_hash The session hash provided by the connecting client.
 * @return True if the hash is authorized.
 */
bool RoutineProgress::CanLogin(const FString& In_hash)
{
    if (Hashes.IsEmpty())
    {
        return false;
    }
    
    return Hashes.Contains(In_hash);
}
