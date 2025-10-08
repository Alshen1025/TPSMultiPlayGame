// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TPSMultiPlayGame/PlayerController/TPSPlayerController.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "TPSMultiPlayGame/TPSComponents/LagCompensationComponent.h"
#include "TPSMultiPlayGame/Weapon/Weapon.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	//발사체의 회전을 이동 방향에 맞춰 자동으로 회전
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
//블루프린트나 에디터에서 값을 변경했을 때 그 변경 사항을 다른 곳에 자동으로 반영해주는 기능
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ATPSCharacter* OwnerCharacter = Cast<ATPSCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		ATPSPlayerController* OwnerController = Cast<ATPSPlayerController>(OwnerCharacter->Controller);
		if (OwnerController)
		{
			if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				//                                       데미지 받을 대상  피해량       공격자      데미지 원인      데미지 타임(기본)
				UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
				//부모 클래스의 OnHIt에서 Destroy가 호출되므로 Super가 맨 마지막에 호출되어야 함
				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}
			//Server-Side Rewind사용
			ATPSCharacter* HitCharacter = Cast<ATPSCharacter>(OtherActor);
			if (bUseServerSideRewind && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled() && HitCharacter)
			{
				OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest(
					HitCharacter,
					TraceStart,
					InitialVelocity,
					OwnerController->GetServerTime() - OwnerController->SingleTripTime
				);
			}
		}
	}
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	/*
	//발사체 예측
	FPredictProjectilePathParams PathParams;
	//지정된 TraceChannel을 사용하여 경로를 따라 트레이스를 수행
	PathParams.bTraceWithChannel = true;
	//사체가 경로를 따라 이동할 때 충돌 검사를 활성화
	PathParams.bTraceWithCollision = true;
	//발사체 경로 그리기
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	//발사 속도-> 방향과 속도를 담은 벡터
	PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
	//예측을 수행할 최대 시간->4초후까지의 경로 계산
	PathParams.MaxSimTime = 4.f;
	//발사체의 반경 크기
	PathParams.ProjectileRadius = 5.f;
	//시뮬레이션의 정밀도를 설정
	PathParams.SimFrequency = 30.f; //초당 30번
	//발사체가 발사되는 시작 위치
	PathParams.StartLocation = GetActorLocation();
	//사용할 트레이스 채널을 지정
	PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	//경로 예측 시 무시할 액터들의 배열
	PathParams.ActorsToIgnore.Add(this);
	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);*/
}
