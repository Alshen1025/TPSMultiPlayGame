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
		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(FireHit.GetActor());
			//몇 발을 맞았는가
			if (TPSCharacter)
			{
				if (HitMap.Contains(TPSCharacter))
				{
					HitMap[TPSCharacter]++;
				}
				else
				{
					HitMap.Emplace(TPSCharacter, 1);
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
		for (auto HitCharacter : HitMap)
		{
			if (HitCharacter.Key && InstigatorController)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{
					//서버는 Rewind가 필요없음
					UGameplayStatics::ApplyDamage(
						HitCharacter.Key,
						Damage * HitCharacter.Value,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
			HitCharacters.Add(HitCharacter.Key);
		}
		if (!HasAuthority() && bUseServerSideRewind)
		{
			// 로그를 식별하기 쉽도록 LogTemp 카테고리에 Warning 레벨로 출력합니다.
			UE_LOG(LogTemp, Warning, TEXT("--- [C++ DEBUG] Client Fire Logic Triggered ---"));

			if (OwnerPawn)
			{
				// UObject의 이름을 얻기 위해 GetName()을 사용합니다.
				UE_LOG(LogTemp, Warning, TEXT("[C++ DEBUG] OwnerPawn: %s (Valid)"), *OwnerPawn->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[C++ DEBUG] OwnerPawn: nullptr (INVALID!)"));
			}

			if (InstigatorController)
			{
				UE_LOG(LogTemp, Warning, TEXT("[C++ DEBUG] InstigatorController: %s (Valid)"), *InstigatorController->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[C++ DEBUG] InstigatorController: nullptr (INVALID!)"));
			}

			ATPSCharacter* LocalOwnerCharacter = Cast<ATPSCharacter>(OwnerPawn);
			ATPSPlayerController* LocalOwnerController = Cast<ATPSPlayerController>(InstigatorController);
			if (LocalOwnerCharacter && LocalOwnerController && LocalOwnerCharacter->GetLagCompensation() && LocalOwnerCharacter->IsLocallyControlled())
			{
				UE_LOG(LogTemp, Warning, TEXT("[C++ DEBUG] All conditions met. Sending ServerScoreRequest RPC..."));
				LocalOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest
				(
					HitCharacters,
					Start,
					HitTargets,
					LocalOwnerController->GetServerTime() - LocalOwnerController->SingleTripTime
				);
			}
			else
			{
				// RPC 전송 실패 시 어떤 조건이 실패했는지 상세히 출력합니다.
				UE_LOG(LogTemp, Error, TEXT("[C++ DEBUG] ServerScoreRequest RPC NOT SENT due to failed conditions:"));
				if (!LocalOwnerCharacter) UE_LOG(LogTemp, Error, TEXT(" -> LocalOwnerCharacter is nullptr"));
				if (!LocalOwnerController) UE_LOG(LogTemp, Error, TEXT(" -> LocalOwnerController is nullptr"));
				if (LocalOwnerCharacter && !LocalOwnerCharacter->GetLagCompensation()) UE_LOG(LogTemp, Error, TEXT(" -> LagCompensationComponent is nullptr"));
				if (LocalOwnerCharacter && !LocalOwnerCharacter->IsLocallyControlled()) UE_LOG(LogTemp, Error, TEXT(" -> Character is not locally controlled"));
			}
			UE_LOG(LogTemp, Warning, TEXT("--- [C++ DEBUG] Client Fire Logic Finished ---\n")); // 로그 구분을 위해 한 줄 띄움
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