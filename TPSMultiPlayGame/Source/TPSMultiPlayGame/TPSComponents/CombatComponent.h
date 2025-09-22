// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TPSMultiPlayGame/HUD/TPSHUD.h"
#include "CombatComponent.generated.h"
#define TRACE_LENGTH 80000.f;

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

	void EquipWeapon(AWeapon* WeaponToEquip);

	friend class ATPSCharacter;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);
	//RPC
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void FireButtonPressed(bool bPressed);

	//FVector_NetQuantize사용이유 -> 발사체의 목표지점을 네트워크를 통해 전송
	//클라이언트가 발사한 위치를 서버에 전송하고 다른 클라이언트에 복제하여 발사체를 동기화.
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	//피격 관련
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float Deltatime);

private:
	ATPSCharacter* Character;
	class ATPSPlayerController* Controller;
	class ATPSHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

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
	void Fire();

	

		
};
