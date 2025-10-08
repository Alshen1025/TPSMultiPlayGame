// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TPSMultiPlayGame/Weapon/Weapontypes.h"
#include "TPSMultiPlayGame/HUD/TPSHUD.h"
#include "TPSMultiPlayGame/TPSTypes/Combatstate.h"
#include "CombatComponent.generated.h"


class AWeapon;
class ATPSCharacter;
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPSMULTIPLAYGAME_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	friend class ATPSCharacter;

	//무기 장착
	void EquipWeapon(AWeapon* WeaponToEquip);

	
	friend class ATPSCharacter;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void FireButtonPressed(bool bPressed);

	//탄약 보충
	void PickupAmmo(EWeaponType Weapon, int32 AmmoAmount);

	//클라이언트 예측 - 재장전
	bool bLocallyReloading = false;

protected:
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);
	//RPC
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	

	//FVector_NetQuantize사용이유 -> 발사체의 목표지점을 네트워크를 통해 전송
	//클라이언트가 발사한 위치를 서버에 전송하고 다른 클라이언트에 복제하여 발사체를 동기화.

	void Fire();
	void FireProjectileWeapon();
	void FireHitScan();
	void FireShotgun();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, float FireDelay);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay);
	UFUNCTION(NetMulticast, Reliable)
	void MultiCasatShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);
	void LocalFire(const  FVector_NetQuantize& TraceHitTarget);
	void ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	//피격 관련
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);
	void SetHUDCrosshairs(float Deltatime);
	//재장전
	UFUNCTION(Server, Reliable)
	void SeverReload();
	UFUNCTION(BlueprintCallable)
	void FinishReload();
	void Reload();
	void HandleReload();
	int32 AmountToReload();


private:
	UPROPERTY()
	ATPSCharacter* Character;

	UPROPERTY()
	class ATPSPlayerController* Controller;

	UPROPERTY()
	class ATPSHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;
	//조준 중인가
	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming = false;
	//조준 버튼을 클릭 하고 있는가
	bool bAimButtonPressed = false;

	UFUNCTION()
	void OnRep_Aiming();

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	//CrossHair 관련 변수
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairShootingFactor;
	float CrosshairAimFactor;
	FHUDPackage HUDPackage;

	//무기 회전
	FVector HitTarget;

	//줌
	//기본 값
	float DefaultFOV;
	//확대시
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	float CurrentFOV;

	void InterpFOV(float DeltaTime);

	//연사기능
	FTimerHandle FireTimer;
	bool bCanFire = true;
	void StartFireTimer();
	void FireTimerFinished();
	bool CanFire();

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	//어떤 무기타입의 탄약을 얼마만큼 가지고 있나.
	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 4;

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 50;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 24;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 10;

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 24;
	//

	void InitalizeCarriedAmmo();

	UPROPERTY(Replicated = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();
	void UpdateCarriedAmmo();

	
	//소유 가능한 최대 탄약
	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 100;
		
};
