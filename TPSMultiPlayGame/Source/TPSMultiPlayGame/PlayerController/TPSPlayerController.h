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
	virtual void OnPossess(APawn* InPawn) override;
protected:
	virtual void BeginPlay() override;

private:
	class ATPSHUD* TPSHUD;
};
