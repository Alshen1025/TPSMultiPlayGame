// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TPSGameState.generated.h"

/**
 * 
 */
UCLASS()
class TPSMULTIPLAYGAME_API ATPSGameState : public AGameState
{
	GENERATED_BODY()
	
public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class ATPSPlayerState* ScoringPlayer);

	UPROPERTY(Replicated)
	TArray<class ATPSPlayerState*>TopScoringPlayers;

private:

	float TopScore = 0.f;
};
