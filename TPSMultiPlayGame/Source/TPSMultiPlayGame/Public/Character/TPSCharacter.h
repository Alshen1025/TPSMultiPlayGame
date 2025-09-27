// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPSMultiPlayGame/TPSTypes/TruningInPlace.h"
#include "TPSMultiPlayGame/Interfaces/InteractWithCrosshairsInterface.h"
#include "TPSMultiPlayGame/TPSTypes/Combatstate.h"
#include "TPSCharacter.generated.h"

UCLASS()
class TPSMULTIPLAYGAME_API ATPSCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ATPSCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_ReplicatedMovement() override;
	virtual void PostInitializeComponents() override;

	//애니메이션 몽타주 재생
	void PlayFireMontage(bool bAiming);
	void PlayHitReactMontage();

	void PlayReloadMontage();
	void PlayDeathMontage();

	//매치 상태에 따라 입력과 행동 제한
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	

protected:
	//관련 클래스 확인 후 HUD 초기화
	void PoolInit();
	virtual void BeginPlay() override;

	//Action, Axis 콜백
	void MoveFoward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void CrouchButtonReleased();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();

	void ReloadButtonPressed();
	void FireButtonReleased();

	virtual void Jump() override;
	//AimOffset관련 계산
	void AimOffset(float DeltaTime);

	//Proxy관련
	void SimProxiesTurn();
	void CalculateAO_Pitch();

/// <summary>
///  플레이어 HP
/// </summary>
	
	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "PlayerStats")
	float Health = 100.f;
	UFUNCTION()
	void OnRep_Health();
	UFUNCTION()
	void ReciveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	void UpdateHUDHealth();

	UPROPERTY()
	class ATPSPlayerController* TPSPlayerController;

	bool bEliminated = false;

	//AimOffset과 기타 회전 관련
	void RotateInPlace(float DeltaTime);


private:
	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();

	//
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	//변수가 복제 될 때 마다 OnRep_OverlappingWeapon 알림
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	//AimOffset 관련 변수
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);




//애니메이션 몽타주
private:

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* DeathMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

public:

//

public:
	//Getter, Setter
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw;  }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }

	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }

	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }

	FORCEINLINE bool GetIsEliminated() const { return bEliminated;  }

	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	ECombatState GetCombatState() const;

 
	AWeapon* GetEquippedWeapon();
	FVector GetHitTarget() const;
	
	//

	//
	void HideCameraIfCharacterClose();
	UPROPERTY(EditAnywhere )
	float CameraThreshold = 200.f;
	//


	//플레이어 제거(사망)
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminated();

	void Eliminated();

public:
	UPROPERTY()
	class ATPSPlayerState* TPSPlayerState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAceess = "true"))
	class UWidgetComponent* OverheadWidget;

	void SetOverlappingWeapon(AWeapon* Weapon);

	bool IsWeaponEquipped();
	bool IsAiming();


	//RepNotify : 변수가 복제될 때 알리는 것
	//DOREPLIFETIME_CONDITION를 통해 Overlapping된 클라이언트에만 복제되므로 그 클라이언트에서만 OnRep_OverlappingWeapon실행
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);
	//RPC(RemoteProcedureCalls)
	//클라이언트가 서버의 함수를 실행시키고 결과 돌려받기
	//무기 장착같은 서버에서 처리하는 로직을 클라이언트도 사용 가능하게
	//Reliable -> 네트워크 상태가 안좋아서 재전송을 해서 꼭 도착하게, 순서 보장으로 속도보단 안정성 우선(꼭 실행되어야 하는 이벤트)
	//Unreliable -> 빠르지만 도착 보장 X, 최신 상태 중요할 때만 사용(플레이어의 위치, 회전같이 자주 바뀌는 정보)
	UFUNCTION(Server, Reliable)
	void ServerEquipButtomPressed();

	//CombatComponent Gatter
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	


	//플레이어 리스폰 관련

	private:

		FTimerHandle ElimTimer;

		UPROPERTY(EditDefaultsOnly)
		float ElimDelay = 3.f;

		void ElimTimerFinishied();

};
