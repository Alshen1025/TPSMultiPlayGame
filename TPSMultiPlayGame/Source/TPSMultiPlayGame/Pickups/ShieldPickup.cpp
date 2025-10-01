// Fill out your copyright notice in the Description page of Project Settings.


#include "ShieldPickup.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "TPSMultiPlayGame/TPSComponents/BuffComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"



AShieldPickup::AShieldPickup()
{
	bReplicates = true;
	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
	PickupEffectComponent->SetupAttachment(RootComponent);
}

void AShieldPickup::OnSphereOverlapping(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlapping(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(OtherActor);
	if (TPSCharacter)
	{
		//PickUp È¿°ú
		UBuffComponent* Buff = TPSCharacter->GetBuff();
		if (Buff)
		{
			Buff->ReplenishShield(ShieldReplenishAmount, ShieldReplenishTime);
		}
	}
	Destroy();
}