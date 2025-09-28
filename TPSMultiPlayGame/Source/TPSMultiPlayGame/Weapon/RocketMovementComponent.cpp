// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketMovementComponent.h"

URocketMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);
	//일반적인 발사체는 부딪히면 멈추거나 튕겨나감
	//AdvanceNextSubstep->물리적으로 멈추지 않고 남은 움직임 계산
	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	//로켓은 멈추면 안 됨; 오직 CollisionBox가 충돌을 감지했을 때 폭발해야 함
	//오버라이드 하고 작성하지 않으면 동작 무시
}