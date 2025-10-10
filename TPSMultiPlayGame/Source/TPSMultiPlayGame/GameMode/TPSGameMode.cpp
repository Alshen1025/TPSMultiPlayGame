// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameMode.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "TPSMultiPlayGame/PlayerController/TPSPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "TPSMultiPlayGame/PlayerState/TPSPlayerState.h"
#include "TPSMultiPlayGame/GameState/TPSGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ATPSGameMode::ATPSGameMode()
{
	//true로 시작시 바로 시작하지 않고 레벨을 날아 다닐 수 있는 defaultpawn을 생성하며 대기함
	bDelayedStart = true;

}

void ATPSGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.0f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}


void ATPSGameMode::BeginPlay()
{
	Super::BeginPlay();
	LevelStartingTime = GetWorld()->GetTimeSeconds();
	
}

void ATPSGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	//Match상태를 플레이어 컨트롤러들에게 알려야함
	//FConstPlayerControllerIterator -> 모든 플레이어 컨트롤러 반복자
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator();  It;  ++It)
	{
		//존재하는 모든 컨트롤러를 순회하면서 MatchState알리기
		ATPSPlayerController* TPSCharacter = Cast<ATPSPlayerController>(*It);
		if (TPSCharacter)
		{
			TPSCharacter->OnMatchStateSet(MatchState, bTeamMatch);
		}
	}
}


//플레이어 사망, 부활
void ATPSGameMode::EliminatePlayer(class ATPSCharacter* EliminatedCharacter, class ATPSPlayerController* VictimController, ATPSPlayerController* AttackerController)
{
	ATPSPlayerState* AttackerPlayerState = AttackerController ? Cast<ATPSPlayerState>(AttackerController->PlayerState) : nullptr;
	ATPSPlayerState* VictimPlayerState = VictimController ? Cast<ATPSPlayerState>(VictimController->PlayerState) : nullptr;
	ATPSGameState* TPSGameState = GetGameState<ATPSGameState>();
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && TPSGameState)
	{
		//리드 중
		TArray<ATPSPlayerState*> PlayersCurrentlyInTheLead;
		for (auto LeadPlayer : TPSGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.Add(LeadPlayer);
		}
		AttackerPlayerState->AddToScore(1.f);
		TPSGameState->UpdateTopScore(AttackerPlayerState);
		if (TPSGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			ATPSCharacter* Leader = Cast<ATPSCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->MulticastGainedTheLead();
			}
		}

		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
		{
			if (!TPSGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				ATPSCharacter* Loser = Cast<ATPSCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser)
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (EliminatedCharacter)
	{
		EliminatedCharacter->Eliminated(false);
	}
	//월드 내에 있는 모든 플레이어 컨트롤러에 ElimAnnouncement를 표시
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ATPSPlayerController* TPSPlayer = Cast<ATPSPlayerController>(*It);
		if (TPSPlayer && AttackerPlayerState && VictimPlayerState)
		{
			TPSPlayer->BroadcastElim(AttackerPlayerState, VictimPlayerState);
		}
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

//TeamGameMode가 아니면 받은 데미지 그대로 반환.
//TeamGameMode에서 아군 식별 코드 추가
float ATPSGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	return BaseDamage;
}

void ATPSGameMode::PlayerLeftGame(ATPSPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;
	ATPSGameState* TPSGameState = GetGameState<ATPSGameState>();
	//게임을 떠난 Player가 TopScoringPlayers에 포함되어있는지
	if (TPSGameState && TPSGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		//포함되어 있으면 제거
		TPSGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}
	ATPSCharacter* CharacterLeaving = Cast<ATPSCharacter>(PlayerLeaving->GetPawn());
	if (CharacterLeaving)
	{
		CharacterLeaving->Eliminated(true);
	}
}


