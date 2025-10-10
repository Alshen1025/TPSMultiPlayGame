// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameState.h"
#include "Net/UnrealNetwork.h"
#include "TPSMultiPlayGame/PlayerState/TPSPlayerState.h"
#include "TPSMultiPlayGame/PlayerController/TPSPlayerController.h"

void ATPSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATPSGameState, TopScoringPlayers);
	DOREPLIFETIME(ATPSGameState, RedTeamScore);
	DOREPLIFETIME(ATPSGameState, BlueTeamScore);
}

void ATPSGameState::UpdateTopScore(ATPSPlayerState* ScoringPlayer)
{
	//점수가 처음으로 들어왔을 때
	//아무도 최고 점수를 기록하지 못함
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	//ScoringPlayer의 점수가 최고 점수와 동일 할 때(동점 처리)
	else if (ScoringPlayer->GetScore() == TopScore)
	{
		//TArray::AddUnique() : 배열에 같은 원소가 이미 있으면 추가하지 않고, 없으면 추가.
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	//플레이어가 최고 점수를 갱신 했을 때
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		//갱신 전 최고 점수를 기록한 플레이어들의 
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}


void ATPSGameState::OnRep_RedTeamScore()
{
	ATPSPlayerController* TPSPlayer = Cast<ATPSPlayerController>(GetWorld()->GetFirstPlayerController());
	if (TPSPlayer)
	{
		TPSPlayer->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ATPSGameState::OnRep_BlueTeamScore()
{
	ATPSPlayerController* TPSPlayer = Cast<ATPSPlayerController>(GetWorld()->GetFirstPlayerController());
	if (TPSPlayer)
	{
		TPSPlayer->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ATPSGameState::RedTeamScores()
{
	++RedTeamScore;
	ATPSPlayerController* TPSPlayer = Cast<ATPSPlayerController>(GetWorld()->GetFirstPlayerController());
	if (TPSPlayer)
	{
		TPSPlayer->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ATPSGameState::BlueTeamScores()
{
	++BlueTeamScore;
	ATPSPlayerController* TPSPlayer = Cast<ATPSPlayerController>(GetWorld()->GetFirstPlayerController());
	if (TPSPlayer)
	{
		TPSPlayer->SetHUDBlueTeamScore(BlueTeamScore);
	}
}
