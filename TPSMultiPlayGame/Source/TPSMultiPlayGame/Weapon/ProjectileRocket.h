// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class TPSMULTIPLAYGAME_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileRocket();
	virtual void Destroyed() override;
	
protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector  NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;
	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	void DestroyTimerFinished();

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;

	UPROPERTY(VisibleAnywhere)
	class URocketMovementComponent* RocketMovementComponent;

private:
	//로켓 매시
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;

	//나이아가라 파괴관련
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
};
