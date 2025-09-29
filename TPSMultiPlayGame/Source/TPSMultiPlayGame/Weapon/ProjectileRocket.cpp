// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "NiagaraSystemInstance.h"
#include "Components/AudioComponent.h"
#include "RocketMovementComponent.h"



AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}



void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}
	//나이아가라 관련
	SpawnTrailSystem();
	//로켓 소리가 반복되게
	if (ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopingSoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}

}





/*
UGameplayStatics::ApplyRadialDamageWithFalloff함수 -> 특정 지점을 중심으로 주변에 방사형 피해를 입힐 때 사용
1.WorldContextObject -> 함수를 호출하는 월드에 대한 컨텍스트 제공
2. BaseDamage -> 최대 피해량, DamageInnerRadius 안에있는 액터가 입는 피해량
3.MinimumDamage ->최소 피해량DamageInnerRadius 가장 자리에 있는 액터가 입게될 최소 피해량
4. Origin->피해가 시작되는 지점 5.DamageInnerRadius -> 최대 피해를 받는 내부 반경
6. DamageOuterRadius -> 외부 반경. 7.DamageFalloff ->피해 감소율 조절 지수
8. DamageTypeClass -> 피해 유형 9. IgnoreActors ->피해 계산 제외 액터 10.DamageCauser ->피해를 입힌 액터
11. 피해를 입힌 플레이어(컨트롤러)
*/
void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//플레이어와 로켓의 충돌 무시
	if (OtherActor == GetOwner())
	{
		return;
	}
	ExplodeDamage();
	StartDestroyTimer();

	//충돌시 효과와 소리, 데미지는 처리되지만 나이아가라 입자 시스템의 Trail은 잠시동안 남아있도록
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}

}

void AProjectileRocket::Destroyed()
{

}

