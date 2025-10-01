// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "TPSMultiPlayGame/Weapon/WeaponTypes.h"
#include "AmmoPickup.generated.h"

/**
 * 
 */
UCLASS()
class TPSMULTIPLAYGAME_API AAmmoPickup : public APickup
{
	GENERATED_BODY()
	
protected:
	virtual void OnSphereOverlapping(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UPROPERTY(EditAnywhere)
	int32 AmmoAmount = 30.f;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
};
