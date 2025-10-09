// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Projectile.h"
#include "Engine/SkeletalMeshSocket.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	UWorld* World = GetWorld();

	if(MuzzleFlashSocket && World)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		//발사될 목표
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = InstigatorPawn;

		AProjectile* SpawnedProjectile = nullptr;
		//ServersideRewind여부 확인
		if (bUseServerSideRewind)
		{
			if (InstigatorPawn->HasAuthority())//Server인지 확인
			{
				if (InstigatorPawn->IsLocallyControlled())//Server이고 Host이기 때문에 복제된 발사체 사용 -> 리슨 서버의 호스트
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>
						(
							ProjectileClass,
							SocketTransform.GetLocation(),
							TargetRotation,
							SpawnParams
						);
					//UseServerSideRewind사용할 필요가 없음
					SpawnedProjectile->bUseServerSideRewind = false;
					SpawnedProjectile->Damage = Damage;
					SpawnedProjectile->HeadShotDamage = HeadShotDamage;
				}
				else //Server이지만 로컬이 아니기 때문에 복제되지 않는 발사체를 생성함. ->데디케이티드 서버
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>
						(
							ServerSideRewindProjectileClass,
							SocketTransform.GetLocation(),
							TargetRotation,
							SpawnParams
						);
					SpawnedProjectile->bUseServerSideRewind = true;
				}
			}
			else //ServerSideRewind를 사용하는 Client
			{
				if (InstigatorPawn->IsLocallyControlled()) //Locallt Controlled Client, 복제되지 않는 발사체 생성 ->로컬 플레이어
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>
						(
							ServerSideRewindProjectileClass,
							SocketTransform.GetLocation(),
							TargetRotation,
							SpawnParams
						);
					//server가 아니므로 SSR 사용해야함
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
				}
				else //SSR를 사용하지 않는 클라이언트 -> 원격 플레이어(프록시)
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}
		else //SSR을 사용하지 않는 Weapon일 경우
		{
			//서버에서 복제되는 발사체 생성
			if (InstigatorPawn->HasAuthority())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->bUseServerSideRewind = false;
				SpawnedProjectile->Damage = Damage;
				SpawnedProjectile->HeadShotDamage = HeadShotDamage;
			}
		}
	}
}
