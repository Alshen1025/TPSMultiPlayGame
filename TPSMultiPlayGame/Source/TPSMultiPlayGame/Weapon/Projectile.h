// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class TPSMULTIPLAYGAME_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Destroyed() override;

	UPROPERTY(EditAnywhere)
	float Damage = 10.f;

	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;

/// <summary>
/// 발사체 Server-Side Rewind에서 사용
/// </summary>

	bool bUseServerSideRewind = false;
	FVector_NetQuantize TraceStart;
	//소수점 2자리 이하
	FVector_NetQuantize100 InitialVelocity;

	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000;

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	void SpawnTrailSystem();

	UFUNCTION()//                                                                                                              <표면으로 향하는 벡터 방향(3D에서 수직을 의미)>
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector  NormalImpulse, const FHitResult& Hit);

	

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	void StartDestroyTimer();
	void DestroyTimerFinished();

	void ExplodeDamage();

	//로켓 매시
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	//폭발 반경
	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;

protected:
	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

public:	
	virtual void Tick(float DeltaTime) override;

private:

	//나이아가라 파괴관련
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
	//

	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;

	class UParticleSystemComponent* TracerComponent;

	

	

};
