// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"


void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;
		if (OwnerController)
		{
			//                                       데미지 받을 대상  피해량       공격자      데미지 원인      데미지 타임(기본)
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
		}
	}
	//부모 클래스의 OnHIt에서 Destroy가 호출되므로 Super가 맨 마지막에 호출되어야 함
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
