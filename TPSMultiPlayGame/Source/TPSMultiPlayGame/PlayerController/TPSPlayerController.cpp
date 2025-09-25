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
