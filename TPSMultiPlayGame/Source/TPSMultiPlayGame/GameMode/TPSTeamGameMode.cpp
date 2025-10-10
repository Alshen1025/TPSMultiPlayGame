// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSTeamGameMode.h"
#include "TPSMultiPlayGame/GameState/TPSGameState.h"
#include "TPSMultiPlayGame/PlayerState/TPSPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "TPSMultiPlayGame/PlayerController/TPSPlayerController.h"


ATPSTeamGameMode::ATPSTeamGameMode()
{
	bTeamMatch = true;
}

void ATPSTeamGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	//플레이어들을 블루, 레드 팀으로 나눔
	//게임 중간에 플레이어가 들어오면 그 플레이어도 팀을 설정해야 함
	ATPSGameState* TPSGameState = Cast<ATPSGameState>(UGameplayStatics::GetGameState(this));
	if (TPSGameState)
	{
		ATPSPlayerState* PlayerState = NewPlayer->GetPlayerState<ATPSPlayerState>();
		if (PlayerState && PlayerState->GetTeam() == ETeam::ET_NoTeam)
		{
			if (TPSGameState->BlueTeam.Num() >= TPSGameState->RedTeam.Num())
			{
				TPSGameState->RedTeam.AddUnique(PlayerState);
				PlayerState->SetTeam(ETeam::ET_RedTeam);
			}
			else
			{
				TPSGameState->BlueTeam.AddUnique(PlayerState);
				PlayerState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}
}

void ATPSTeamGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	//현재 게임내에 존재하는 플레이어들을 블루, 레드 팀으로 나눔
	ATPSGameState* TPSGameState = Cast<ATPSGameState>(UGameplayStatics::GetGameState(this));
	if (TPSGameState)
	{
		for (auto Player : TPSGameState->PlayerArray)
		{
			ATPSPlayerState* PlayerState = Cast<ATPSPlayerState>(Player.Get());
			if (PlayerState && PlayerState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (TPSGameState->BlueTeam.Num() >= TPSGameState->RedTeam.Num())
				{
					TPSGameState->RedTeam.AddUnique(PlayerState);
					PlayerState->SetTeam(ETeam::ET_RedTeam);
				}
				else
				{
					TPSGameState->BlueTeam.AddUnique(PlayerState);
					PlayerState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}
}

void ATPSTeamGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	ATPSGameState* TPSGameState = Cast<ATPSGameState>(UGameplayStatics::GetGameState(this));
	ATPSPlayerState* PlayerState = Exiting->GetPlayerState<ATPSPlayerState>();
	if (TPSGameState && PlayerState)
	{
		if (TPSGameState->RedTeam.Contains(PlayerState))
		{
			TPSGameState->RedTeam.Remove(PlayerState);
		}
		if (TPSGameState->BlueTeam.Contains(PlayerState))
		{
			TPSGameState->BlueTeam.Remove(PlayerState);
		}
	}
}


float ATPSTeamGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	ATPSPlayerState* AttackerPState = Attacker->GetPlayerState<ATPSPlayerState>();
	ATPSPlayerState* VictimPState = Victim->GetPlayerState<ATPSPlayerState>();
	if (AttackerPState == nullptr || VictimPState == nullptr) return BaseDamage;
	if (VictimPState == AttackerPState)
	{
		return BaseDamage;
	}
	if (AttackerPState->GetTeam() == VictimPState->GetTeam())
	{
		return 0.f;
	}
	return BaseDamage;
}

void ATPSTeamGameMode::EliminatePlayer(ATPSCharacter* EliminatedCharacter, ATPSPlayerController* VictimController, ATPSPlayerController* AttackerController)
{
	Super::EliminatePlayer(EliminatedCharacter, VictimController, AttackerController);
	ATPSGameState* TPSGameState = Cast<ATPSGameState>(UGameplayStatics::GetGameState(this));
	ATPSPlayerState* AttackerPlayerState = AttackerController ? Cast<ATPSPlayerState>(AttackerController->PlayerState) : nullptr;
	if (TPSGameState && AttackerPlayerState)
	{
		if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			TPSGameState->BlueTeamScores();
		}
		if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			TPSGameState->RedTeamScores();
		}
	}
}
