// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "TPSMultiPlayGame/TPSComponents/BuffComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"


AHealthPickup::AHealthPickup()
{
	bReplicates = true;
	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
	PickupEffectComponent->SetupAttachment(RootComponent);
}

void AHealthPickup::Destroyed()
{
	//¿Ã∆Â∆Æ
	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickupEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}
	Super::Destroyed();
}

void AHealthPickup::OnSphereOverlapping(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlapping(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(OtherActor);
	if (TPSCharacter)
	{
		//PickUp »ø∞˙
		UBuffComponent* Buff = TPSCharacter->GetBuff();
		if (Buff)
		{
			Buff->Heal(HealAmount, HealingTime);
		}
	}
	Destroy();
}
