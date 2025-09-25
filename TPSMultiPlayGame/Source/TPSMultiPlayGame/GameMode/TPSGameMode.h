// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TPSGameMode.generated.h"

/**
 * 
 */
UCLASS()
class TPSMULTIPLAYGAME_API ATPSGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void EliminatePlayer(class ATPSCharacter* EliminatedCharacter, class ATPSPlayerController* VictimController, ATPSPlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);
	
};
