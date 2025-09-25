// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameMode.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "TPSMultiPlayGame/PlayerController/TPSPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "TPSMultiPlayGame/PlayerState/TPSPlayerState.h"



//플레이어 사망, 부활
void ATPSGameMode::EliminatePlayer(class ATPSCharacter* EliminatedCharacter, class ATPSPlayerController* VictimController, ATPSPlayerController* AttackerController)
{
	ATPSPlayerState* AttackerPlayerState = AttackerController ? Cast<ATPSPlayerState>(AttackerController->PlayerState) : nullptr;
	ATPSPlayerState* VictimPlayerState = VictimController ? Cast<ATPSPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (EliminatedCharacter)
	{
		EliminatedCharacter->Eliminated();
	}
}

void ATPSGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
