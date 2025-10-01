// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "ShieldPickup.generated.h"

/**
 * 
 */
UCLASS()
class TPSMULTIPLAYGAME_API AShieldPickup : public APickup
{
	GENERATED_BODY()
	
public:
	AShieldPickup();

protected:
	virtual void OnSphereOverlapping(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* PickupEffectComponent;

	UPROPERTY(EditAnywhere)
	float ShieldReplenishAmount = 100.f;

	UPROPERTY(EditAnywhere)
	float ShieldReplenishTime = 5.f;
};
