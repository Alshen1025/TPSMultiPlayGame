// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayerController.h"
#include "TPSMultiPlayGame/HUD/TPSHUD.h"
#include "TPSMultiPlayGame/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"

void ATPSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	TPSHUD = Cast<ATPSHUD>(GetHUD());
}


void ATPSPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SetHUDTime();
	CheckTimeSync(DeltaTime);
}

void ATPSPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

//TODO : 서버와 클라이언트의 시간다름

void ATPSPlayerController::SetHUDTime()
{
	//남은 시간 = MatchTIme(설정한 게임 시간) - 게임이 시작되고 흐른 시간
	uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime());
	if (CountdownInt != SecondsLeft)
	{
		SetHUDMatchCountdown(MatchTime - GetServerTime());
	}
	CountdownInt = SecondsLeft;
}


float ATPSPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds(); //서버면 그냥 현재 시간 쓰면 됨
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ATPSPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ATPSPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceiced)
{
	//요청을 보내고 답을 받을 때 까지 걸린 시간(서버 - 클라이언트 왕복시간)
	//클라이언트가 서버에 요청을 보내고 얼마나 많은 시간이 흘렀는가
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;

	//서버의 현재 시간을 계산
	//서버가 클라이언트의 요청을 받은시간에 왕복 시간의 절반을 더하기
	float  CurrentSeverTime = TimeServerReceiced + (0.5 * RoundTripTime);

	//서버의 현재 시간에서 클라이언트의 현재 시간을 빼면 서버와 클라이언트 사이의 시간차이 계산 가능
	ClientServerDelta = CurrentSeverTime - GetWorld()->GetTimeSeconds();
}

//서버와 클라이언트 서버 시간 동기화
void ATPSPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	//서버는 자신의 현재 시간을 가져옴
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	//클라이언트 RPC호출해 서버 시간 전달
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

//

void ATPSPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;

	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->HealthBar &&
		TPSHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		TPSHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		TPSHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void ATPSPlayerController::SetHUDScore(float Score)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		TPSHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}

}

void ATPSPlayerController::SetHUDDefeats(int32 Defeats)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		TPSHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
}

void ATPSPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		TPSHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ATPSPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
		TPSHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ATPSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(InPawn);
	if (TPSCharacter)
	{
		SetHUDHealth(TPSCharacter->GetHealth(), TPSCharacter->GetMaxHealth());
	}
}


void ATPSPlayerController::SetHUDMatchCountdown(float CountDownTime)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		int32 Minutes = FMath::FloorToInt(CountDownTime / 60.f);
		int32 Seconds = CountDownTime - Minutes * 60;


		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		TPSHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}

}
