// Fill out your copyright notice in the Description page of Project Settings.


#include "RoutineProgress.h"
#include "CompactBinaryReader.h"
#include "OptimizedBinaryPacketSerializer.h"
#include "PacketStructure.h"

TUniquePtr<RoutineProgress> GRoutineProgress =MakeUnique<RoutineProgress>();
RoutineProgress::RoutineProgress()
{
}

RoutineProgress::~RoutineProgress()
{
}

void RoutineProgress::HandleReceivedPacket(const TArray<uint8_t>& data)
{
	TUniquePtr<CompactBinaryReader> reader=MakeUnique<CompactBinaryReader>();
	
	TArray<uint8> Deserializedata;
	PacketType packet_type;

	if (!OptimizedBinaryPacketSerializer::ParseSecurePacket(data,packet_type,reader.Get())||!reader.Get())
	{
		return;
	}
	switch (packet_type)
	{
	case PacketType::GameCreate:
		{
		
			FUserAuthData Data;
			OptimizedBinaryPacketSerializer::DeserializePacket<FUserAuthData>(*reader,Data);
			if (Data.hash.IsEmpty())return;
			Hashes=Data.hash;
			int i=1;
			for (const auto&it:Hashes)
			{
				UE_LOG(LogTemp, Error, TEXT("%d:번째 해쉬: %s"),i,*it);
				i++;
			}
		
			break;
		}
		default:
		break;
	}
	UE_LOG(LogTemp, Error, TEXT("HandleReceivedPacket 끝!"));
}
TArray<FString> RoutineProgress::GetHashes() const
{
	return Hashes;
   
}

TArray<FString> RoutineProgress::GetNickNames() const
{
	return  NickNames;
}

void RoutineProgress::SetHashes(const TArray<FString>& In_Hashs)
{
	Hashes=In_Hashs;
}

void RoutineProgress::SetNickNames(const TArray<FString>& In_NickNames)
{
	NickNames=In_NickNames;
}

bool RoutineProgress::CanLogin(const FString& In_hash)
{
	if (Hashes.IsEmpty())return false;
	
	return Hashes.Contains(In_hash);
}
