// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TPSCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "TPSMultiPlayGame/Weapon/Weapon.h"
#include "Net/UnrealNetwork.h"
#include "TPSMultiPlayGame/TPSComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ATPSCharacter::ATPSCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OvergeadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	//������Ʈ ��
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}


void ATPSCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AimOffset(DeltaTime);
}

void ATPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ATPSCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ATPSCharacter::EquipButtonPressed);

	PlayerInputComponent->BindAxis("MoveForward", this, &ATPSCharacter::MoveFoward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATPSCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &ATPSCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ATPSCharacter::LookUp);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ATPSCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ATPSCharacter::CrouchButtonReleased);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ATPSCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ATPSCharacter::AimButtonReleased);
}

void ATPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//������ ������ ���� Ŭ������ ������ ����
	//��� Ŭ���̾�Ʈ�� ������
	//DOREPLIFETIME(ATPSCharacter, OverlappingWeapon);

	//���⿡ Overlapping�� Ŭ���̾�Ʈ�� �������� ����
	DOREPLIFETIME_CONDITION(ATPSCharacter, OverlappingWeapon, COND_OwnerOnly);
}

void ATPSCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}

void ATPSCharacter::MoveFoward(float Value)
{
	if (Controller != nullptr && Value != 0.0f)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}
void ATPSCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.0f)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}
void ATPSCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ATPSCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}



void ATPSCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

AWeapon* ATPSCharacter::GetEquippedWeapon()
{
	if(Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

//TODO : Turn���� �ִϸ��̼� ã��
void ATPSCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
}

void ATPSCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	//���⿡ ��ģ�� ������ ��
	if (IsLocallyControlled())
	{
		if(OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ATPSCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ATPSCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

void ATPSCharacter::ServerEquipButtomPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ATPSCharacter::EquipButtonPressed()
{
	//���� ���� ��ư�� ������ �� ������ ��� �׳� �����ϰ�
	//Ŭ���̾�Ʈ�� RPCȣ��
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtomPressed();
		}
	}
}

void ATPSCharacter::CrouchButtonPressed()
{
	Crouch();
}

void ATPSCharacter::CrouchButtonReleased()
{
	UnCrouch();
}

void ATPSCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ATPSCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ATPSCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	//�ӵ� ���
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	if (Speed == 0.0f && !bIsInAir) //Standing �����̰� �����ϰ� ���� ���� ��
	{
		FRotator CurrentAimRotation =  FRotator(0.f, GetBaseAimRotation().Yaw, 0.0f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = false;

	}
	if (Speed > 0.f || bIsInAir) //running�̰ų� jump�����϶�
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.0f);
		AO_Yaw = 0.0;
		bUseControllerRotationYaw = true;
	}
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.0f && !IsLocallyControlled())
	{
		//pitch�� (270, 360)���� (-90, 0)���� ����
		FVector2D InRange(270.0f, 360.0f);
		FVector2D OutRange(-90.0f, 0.0f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}




