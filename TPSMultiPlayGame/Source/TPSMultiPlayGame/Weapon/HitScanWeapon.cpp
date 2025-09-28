// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	//무기를 발사하는 주체
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	//InstigatorController는 다른 Simulated Proxy에서는 null

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		//라인 트레이스 시작점과 끝점
		FVector Start = SocketTransform.GetLocation();
		FVector End = Start + (HitTarget - Start) * 1.25f;

		//라인 트레이스 시작
		FHitResult FireHit;
		UWorld* World = GetWorld();
		if (World)
		{
			World->LineTraceSingleByChannel
			(
				FireHit,
				Start,
				End,
				ECollisionChannel::ECC_Visibility
			);
			//Beam용 로컬벡터
			FVector BeamEnd = End;
			//Hit이 발생했으면
			if (FireHit.bBlockingHit)
			{
				BeamEnd = FireHit.ImpactPoint;
				//Damage주기
				//발사체가 없으므로 라인트레이스 발생 시 데미지를 가함.
				//TPSCharacter->피해를 입을 대상
				ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(FireHit.GetActor());
				if (TPSCharacter && HasAuthority() && InstigatorController)
				{
					//데미지 처리는 서버에서만
					UGameplayStatics::ApplyDamage(
						TPSCharacter,
						Damage,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						World,
						ImpactParticles,
						FireHit.ImpactPoint,
						FireHit.ImpactNormal.Rotation()
					);
				}
				if (BeamParticles)
				{
					UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
						World,
						BeamParticles,
						SocketTransform
					);
					if (Beam)
					{
						Beam->SetVectorParameter(FName("Target"), BeamEnd);
					}
				}
			}
		}
	}
}
