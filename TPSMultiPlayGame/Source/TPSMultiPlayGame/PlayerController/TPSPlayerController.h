// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "TPSPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class TPSMULTIPLAYGAME_API ATPSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 WeaponAmmo);
	void SetHUDCarriedAmmo(int32 CarriedAmmo);
	void SetHUDMatchCountdown(float CountDownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Single-Trip Time
	float SingleTripTime = 0;

	FHighPingDelegate HighPingDelegate;

	//처치 알림
	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);

protected:

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);
	virtual void BeginPlay() override;
	void SetHUDTime();

	//Return to Main Menu
	virtual void SetupInputComponent() override;

	void ShowReturnToMainMunu(const FInputActionValue& Value);
	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf <class UUserWidget> ReturnToMainMenuWidget;

	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* Quit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	bool bReturnToMainMenuOpen = false;

/// <summary>
/// 클라이언트와 서버간의 시간 동기화
/// </summary>

	//현재 서버시간 요청
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	//클라이언트에게 현재 서버시간 알림.
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceiced );

	//클라이언트와 서버의 시간 차이
	float ClientServerDelta = 0.f;

	//주기적으로 서버 시간과 동기화
	//주기를 어느 정도로 할 것인가
	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;  //5초

	//마지막 동기화 이후 경과한 시간
	float TimeSyncRunningTime = 0.0f;

	void CheckTimeSync(float DeltaTime);

	void PollInit();

	//플레이어가 매치 상태를 알 수 있도록(게임 중간에 참여하는 경우)
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	//클라이언트의 게임 참가
	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName StateOfMatch, float Warmup, float Match, float StartingTime, float Cooldown);

	//Ping관련 경고
	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);

public:
	
	virtual float GetServerTime(); //서버의 시간으로 동기화
	virtual void ReceivedPlayer() override;  //가능한 한 빠리 서버 시계와 동기화
	void OnMatchStateSet(FName State); //Match State설정

	//매치 시작
	void HandleMatchHasStarted();
	//매치 종료
	void HandleCooldown();


/// <summary>
/// 
/// </summary>

private:
	UPROPERTY()
	class ATPSHUD* TPSHUD;

	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	bool bInitializeHealth = false;
	bool bInitializeScore = false;
	bool bInitializeDefeats = false;
	bool bInitializeShield = false;

	//HUD
	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
	float HUDShield;
	float HUDMaxShield;


private:
	//Ping관련
	float HighPingRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;

	float PingAnimationRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 3.0f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;

};
