// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
#include "TPSMultiPlayGame/PlayerController/TPSPlayerController.h"
#include "TPSMultiPlayGame/TPSComponents/LagCompensationComponent.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector());
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		//샷건의 탄환이 여러 사람을 동시에 맞추는 경우 고려
		TMap<ATPSCharacter*, uint32> HitMap;
		TMap<ATPSCharacter*, uint32> HeadShotHitMap;
		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(FireHit.GetActor());
			//몇 발을 맞았는가
			if (TPSCharacter)
			{
				const bool bHeadShot = FireHit.BoneName.ToString() == FString("head");

				if (bHeadShot)
				{
					if (HeadShotHitMap.Contains(TPSCharacter)) HeadShotHitMap[TPSCharacter]++;
					else HeadShotHitMap.Emplace(TPSCharacter, 1);
				}
				else
				{
					if (HitMap.Contains(TPSCharacter)) HitMap[TPSCharacter]++;
					else HitMap.Emplace(TPSCharacter, 1);
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
		}

		
		//각 플레이어가 맞은 탄환의 수 만큼 데미지 계산
		TArray<ATPSCharacter*> HitCharacters;
		TMap<ATPSCharacter*, float> DamageMap;

		//피격 횟수와  데미지를 곱하여 헤드샷 데미지를 계산하고, DamageMap에 저장
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key)
			{
				DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);

				HitCharacters.AddUnique(HitPair.Key);
			}
		}
		//피격 횟수와 헤드샷 데미지를 곱하여 헤드샷 데미지를 계산하고, DamageMap에 저장
		for (auto HeadShotHitPair : HeadShotHitMap)
		{
			if (HeadShotHitPair.Key)
			{
				if (DamageMap.Contains(HeadShotHitPair.Key)) DamageMap[HeadShotHitPair.Key] += HeadShotHitPair.Value * HeadShotDamage;
				else DamageMap.Emplace(HeadShotHitPair.Key, HeadShotHitPair.Value * HeadShotDamage);

				HitCharacters.AddUnique(HeadShotHitPair.Key);
			}
		}

		for (auto Damagepair : DamageMap)
		{
			if (Damagepair.Key && InstigatorController)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{
					//서버는 Rewind가 필요없음
					UGameplayStatics::ApplyDamage(
						Damagepair.Key, //피격된 캐릭터
						Damagepair.Value, //캐릭터가 입으 총 데미지
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}
		if (!HasAuthority() && bUseServerSideRewind)
		{
			ATPSCharacter* LocalOwnerCharacter = Cast<ATPSCharacter>(OwnerPawn);
			ATPSPlayerController* LocalOwnerController = Cast<ATPSPlayerController>(InstigatorController);
			if (LocalOwnerCharacter && LocalOwnerController && LocalOwnerCharacter->GetLagCompensation() && LocalOwnerCharacter->IsLocallyControlled())
			{
				LocalOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest
				(
					HitCharacters,
					Start,
					HitTargets,
					LocalOwnerController->GetServerTime() - LocalOwnerController->SingleTripTime
				);
			}
		}
	}
}


void AShotgun::ShotgunTraceEndWithScatter(const FVector & HitTarget, TArray<FVector_NetQuantize>&HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();
		HitTargets.Add(ToEndLoc);
	}
}