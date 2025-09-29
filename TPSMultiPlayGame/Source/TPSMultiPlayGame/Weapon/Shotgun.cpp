// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "Engine/SkeletalMeshSocket.h"


void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		//산탄은 여러 데미지가 한번에 발생하므로 하나로 합친다음 전달
		uint32 Hits = 0;
		//샷건의 탄환이 여러 사람을 동시에 맞추는 경우 고려
		TMap<ATPSCharacter*, uint32> HitMap;

		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			FHitResult FireHit;
			//피격 판정
			WeaponTraceHit(Start, HitTarget, FireHit);
			ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(FireHit.GetActor());

			//몇 발을 맞았는가
			if (TPSCharacter && HasAuthority() && InstigatorController)
			{
				if (HitMap.Contains(TPSCharacter))
				{
					HitMap[TPSCharacter]++;
				}
				else
				{
					HitMap.Emplace(TPSCharacter, 1);
				}
			}
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation()
				);
			}
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					0.5f,
					FMath::FRandRange(-0.5f, 0.5f)
				);
			}
		}
		//각 플레이어가 맞은 탄환의 수 만큼 데미지 계산
		for (auto HitCharacter : HitMap)
		{
			if (HitCharacter.Key && HasAuthority() && InstigatorController)
			{
				//데미지 처리는 서버에서만
				UGameplayStatics::ApplyDamage(
					HitCharacter.Key,
					Damage * HitCharacter.Value,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
		}
	}
}
