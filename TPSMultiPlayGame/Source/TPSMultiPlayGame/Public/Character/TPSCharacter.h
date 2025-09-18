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

	//Action, Axis �ݹ�
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

	//������ ���� �� �� ���� OnRep_OverlappingWeapon �˸�
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


	//RepNotify : ������ ������ �� �˸��� ��
	//DOREPLIFETIME_CONDITION�� ���� Overlapping�� Ŭ���̾�Ʈ���� �����ǹǷ� �� Ŭ���̾�Ʈ������ OnRep_OverlappingWeapon����
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);
	//RPC(RemoteProcedureCalls)
	//Ŭ���̾�Ʈ�� ������ �Լ��� �����Ű�� ��� �����ޱ�
	//���� �������� �������� ó���ϴ� ������ Ŭ���̾�Ʈ�� ��� �����ϰ�
	//Reliable -> ��Ʈ��ũ ���°� �����Ƽ� �������� �ؼ� �� �����ϰ�, ���� �������� �ӵ����� ������ �켱(�� ����Ǿ�� �ϴ� �̺�Ʈ)
	//Unreliable -> �������� ���� ���� X, �ֽ� ���� �߿��� ���� ���(�÷��̾��� ��ġ, ȸ������ ���� �ٲ�� ����)
	UFUNCTION(Server, Reliable)
	void ServerEquipButtomPressed();

};
