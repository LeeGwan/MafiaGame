// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class MAFIAGAME_API RoutineProgress
{
public:
	RoutineProgress();
	~RoutineProgress();
	void HandleReceivedPacket(const TArray<uint8_t>& data);
	TArray<FString> GetHashes()const;
	TArray<FString> GetNickNames()const;
	void SetHashes(const TArray<FString>& In_Hashs);
	void SetNickNames(const TArray<FString>& In_NickNames);
	bool CanLogin(const FString& In_hash);
private:
	TArray<FString> Hashes;
	TArray<FString> NickNames;
};
extern TUniquePtr<RoutineProgress> GRoutineProgress;
