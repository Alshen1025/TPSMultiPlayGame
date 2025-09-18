// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPSCharacter.generated.h"

UCLASS()
class TPSMULTIPLAYGAME_API ATPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ATPSCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

protected:
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


private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	//변수가 복제 될 때 마다 OnRep_OverlappingWeapon 알림
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;
public:
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

};
