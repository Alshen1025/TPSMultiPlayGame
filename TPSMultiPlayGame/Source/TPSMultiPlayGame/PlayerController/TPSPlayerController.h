// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TPSPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class TPSMULTIPLAYGAME_API ATPSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 WeaponAmmo);
	void SetHUDCarriedAmmo(int32 CarriedAmmo);
	void SetHUDMatchCountdown(float CountDownTime);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
	void SetHUDTime();

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

public:
	
	virtual float GetServerTime(); //서버의 시간으로 동기화
	virtual void ReceivedPlayer() override;  //가능한 한 빠리 서버 시계와 동기화

/// <summary>
/// 
/// </summary>

private:
	UPROPERTY()
	class ATPSHUD* TPSHUD;

	float MatchTime = 120.f;
	uint32 CountdownInt = 0;
};
