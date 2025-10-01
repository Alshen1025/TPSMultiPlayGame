// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "TPSMultiPlayGame/TPSComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlapping(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlapping(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(OtherActor);
	if (TPSCharacter)
	{
		UCombatComponent* Combat = TPSCharacter->GetCombat();
		if (Combat)
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	Destroy();
}
