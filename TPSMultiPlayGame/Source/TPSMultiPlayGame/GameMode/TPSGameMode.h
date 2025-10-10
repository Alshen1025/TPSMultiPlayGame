// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TPSGameMode.generated.h"

//커스텀 MatchState
namespace MatchState
{
	//게임 종료 후 대기시간
	extern TPSMULTIPLAYGAME_API const FName Cooldown;
}


UCLASS()
class TPSMULTIPLAYGAME_API ATPSGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATPSGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void EliminatePlayer(class ATPSCharacter* EliminatedCharacter, class ATPSPlayerController* VictimController, ATPSPlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);

	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);

	//GameState관련
	//Warmup상태에서 일정 시간이 지난 후 게임 시작
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	//게임 시간
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	float LevelStartingTime = 0.f;

	//게임 종료 후 대기시간
	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.0f;

	bool bTeamMatch = false;

	FORCEINLINE float GetCountdownTime() const {return CountdownTime;}

	void PlayerLeftGame(class ATPSPlayerState* PlayerLeaving);

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;


private:
	float CountdownTime = 0.f;
	//
};
