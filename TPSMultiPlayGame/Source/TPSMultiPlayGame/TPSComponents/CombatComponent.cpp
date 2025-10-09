


#include "CombatComponent.h"
#include "TPSMultiPlayGame/Weapon/Weapon.h"
#include "TPSMultiPlayGame/Weapon/HitScanWeapon.h"
#include "TPSMultiPlayGame/Weapon/Shotgun.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "TPSMultiPlayGame/PlayerController/TPSPlayerController.h"
#include "TimerManager.h"
#include "Camera/CameraComponent.h"
#include "Sound/SoundCue.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 400.f;
	bCanFire = true;
	// ...
}
void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, CombatState);
	//소유 클라이언트에만 복제
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	
}
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (Character->HasAuthority())
		{
			InitalizeCarriedAmmo();
		}
	}
}
void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
	//클라이언트 예측 - Aiming
	if(Character->IsLocallyControlled()) bAimButtonPressed = bAiming;
}
void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}
void UCombatComponent::OnRep_Aiming()
{
	//서버에서 복제된 변수(bAiming)을 확인하기 전까지 클라이언트 변수 bAimButtonPressed로 상태를 예측해서 미리 적용
	if (Character && Character->IsLocallyControlled())
	{
		bAiming = bAimButtonPressed;
	}
}
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	//이미 무기를 장착 중이면 보유중인 무기를 Drop
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
	
	//장착
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket= Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	//장착 사운드
	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation
		(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
		);
	}
	//빈 무기면 자동 재장전
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}

	//
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		if (EquippedWeapon->EquipSound)
		{
			UGameplayStatics::PlaySoundAtLocation
			(
				this,
				EquippedWeapon->EquipSound,
				Character->GetActorLocation()
			);
		}
	}
}
//재장전 관련


void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	//장전 후에 소지한 탄약 개수 차감
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ATPSPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(ReloadAmount);
}
void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;
	//휴대 탄약
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ATPSPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::SeverReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;


	CombatState = ECombatState::ECS_Reloading;
	if(!Character->IsLocallyControlled()) HandleReload();

}
void UCombatComponent::FinishReload()
{
	if (Character == nullptr) return;
	bLocallyReloading = false;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		//탄약 업데이트
		UpdateAmmoValues();
	}
	if (bFireButtonPressed)
	{
		Fire();
	}
	
}
void UCombatComponent::HandleReload()
{
	if (Character)
	{
		Character->PlayReloadMontage();
	}
	
}
void UCombatComponent::Reload()
{
	if (EquippedWeapon == nullptr) return;
	//탄창에 빈 공간이 얼마나 되는지 계산
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	//보유 탄약이 존재 하고 재장전 중이 아닐때
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading && !bLocallyReloading)
	{
		SeverReload();
		//재장전 애니메이션 바로 출력
		HandleReload();
		bLocallyReloading = true;
	}
}
int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	//탄창에 빈 공간이 얼마나 되는지 계산
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		//사용자가 가지고 있는 탄약 수 얻어오기
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];

		//두 숫자 중 더 작은 값 반환
		//재장전 가능한 발 수 제한
		int32 Least = FMath::Min(RoomInMag, AmountCarried);

		// clamp(값, 최소, 최대)
		//값을 최소와 최대 범위 안에 있도록 강제
		//가지고 있는 탄보다 많은 탄약 장전 불가
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}


///
///          사격 관련
///
/// 
/// 
void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		if (EquippedWeapon)
		{
			CrosshairShootingFactor = .75f;

			switch (EquippedWeapon->FireType)
			{
			case EFireType::EFT_Projectile:
				FireProjectileWeapon();
				break;
			case EFireType::EFT_HitScan:
				FireHitScan();
				break;
			case EFireType::EFT_Shotgun:
				FireShotgun();
				break;
			}
		}
		StartFireTimer();
	}
}
void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if(!Character->HasAuthority()) LocalFire(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->FireDelay);
	}
}
void UCombatComponent::FireHitScan()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!Character->HasAuthority()) LocalFire(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->FireDelay);
	}
}
void UCombatComponent::FireShotgun()
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun && Character)
	{
		TArray<FVector_NetQuantize> HitTargets;
		Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets);
		if (!Character->HasAuthority()) ShotgunLocalFire(HitTargets);
		ServerShotgunFire(HitTargets, EquippedWeapon->FireDelay);
	}
}
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	MulticastFire(TraceHitTarget);
}
bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	if (EquippedWeapon)
	{
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}
//MulticastRPC
//서버에서 호출하면 서버, 크라이언트에서 모두 실행되는 함수
void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	LocalFire(TraceHitTarget);
}
void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	MultiCasatShotgunFire(TraceHitTargets);
}
bool UCombatComponent::ServerShotgunFire_Validate(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	if (EquippedWeapon)
	{
		//서버로 전달된 FireDelay값과 무기의 FireDelay값을 비교해서 거의 같으면 유효한 요청으로 인정
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}
void UCombatComponent::MultiCasatShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	ShotgunLocalFire(TraceHitTargets);
}
//총기 발사 시 발사 애니메이션, 효과, 소리는 서버가 아니라 클라이언트에서 자체적으로 발생하도록 하고
//총기 발사, 피격같은 핵심 로직만 서버에서 관리하도록
void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}
void UCombatComponent::ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun == nullptr || Character == nullptr) return;
	if (CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_Unoccupied)
	{
		bLocallyReloading = false;
		Character->PlayFireMontage(bAiming);
		Shotgun->FireShotgun(TraceHitTargets);
		CombatState = ECombatState::ECS_Unoccupied;
	}
}
void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;
	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}
void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}
void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if(bFireButtonPressed)
	{
		Fire();
	}
	
}
//탄약 보충 함수
void UCombatComponent::PickupAmmo(EWeaponType Weapon, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(Weapon))
	{
		CarriedAmmoMap[Weapon] = FMath::Clamp(CarriedAmmoMap[Weapon] + AmmoAmount, 0 , MaxCarriedAmmo);
		UpdateCarriedAmmo();
	}
}
bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	if (bLocallyReloading) return false;
	return (!EquippedWeapon->IsEmpty() && bCanFire && CombatState==ECombatState::ECS_Unoccupied  && !Character->GetCharacterMovement()->IsFalling());
}
///
///사용자 휴대 탄약
void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ATPSPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}
void UCombatComponent::InitalizeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}
void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	case ECombatState::ECS_Reloading:
		if (Character && !Character->IsLocallyControlled()) HandleReload();
		break;
	case ECombatState::ECS_MAX:
		break;
	}
}
//
//피격 판정 관련
void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	//조준점(화면 중앙)에서 RayTrace로 조준점에 맞았는지 판정(임시)
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.);

	//십자선의 위치와 방향
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld
	(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		//라인 트레이스가 카메라에서 시작되어서 카메라 와 플레이어 사이에 물체가 있거나 카메라가 회전 해서 플레이어와 겹칠 때 추척하던 현상 개선
		//라인 트레이스 시작이 플레이어 캐릭터의 앞이 되게 수정
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		GetWorld()->LineTraceSingleByChannel
		(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
			
		);
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}

	}
}

void UCombatComponent::SetHUDCrosshairs(float Deltatime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;

	Controller = Controller == nullptr ? Cast<ATPSPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ATPSHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
			}
			HUD->SetHUDPackage(HUDPackage);

			

			//조준점 스프레드(이동시 조준점 확대) 계산
			// 속도 범위: 0 ~ 캐릭터 최대 걷기 속도
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			// 변환할 값의 범위: 0 ~ 1
			FVector2D VelocityMultiplierRange(0.f, 1.f);

			//현재 속도 계산
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			//속도를 0~1사이 값으로 변환
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			//공중에 떠있으면
			// CrosshairInAirFactor 값을 목표치인 2.25f까지 부드럽게 증가
			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, Deltatime, 2.25f);
			}
			// 캐릭터가 땅에 있다면
			// CrosshairInAirFactor 값을 목표치인 0.f까지 부드럽게 감소
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, Deltatime, 30.f);
			}
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, Deltatime, 40.f);
			HUDPackage.CrosshairSpread = CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairInAirFactor + CrosshairShootingFactor;

			HUD->SetHUDPackage(HUDPackage);
		}
		
	}

}


void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}




