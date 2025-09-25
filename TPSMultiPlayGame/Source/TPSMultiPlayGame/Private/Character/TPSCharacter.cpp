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
#include "Character/TPSAnimInstance.h"
#include "TPSMultiPlayGame/TPSMultiPlayGame.h"
#include "TPSMultiPlayGame/PlayerController/TPSPlayerController.h"
#include "TPSMultiPlayGame/GameMode/TPSGameMode.h"
#include "TimerManager.h"
#include "TPSMultiPlayGame/PlayerState/TPSPlayerState.h"
#include "TPSMultiPlayGame/Weapon/WeaponTypes.h"

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
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	//업데이트 빈도
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}


FVector ATPSCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}


//플레이어가 벽에 가까이 붙었을 때 플레이어 와 무기 Mesh가 화면을 가리지 않게
void ATPSCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if ( (FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold )
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon)
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon)
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ATPSCharacter::PoolInit()
{
	if (TPSPlayerState == nullptr)
	{
		TPSPlayerState = GetPlayerState<ATPSPlayerState>();
		if (TPSPlayerState)
		{
			TPSPlayerState->AddToScore(0.f);
			TPSPlayerState->AddToDefeats(0);
		}
	}
}

void ATPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	
	TPSPlayerController = Cast<ATPSPlayerController>(Controller);
	if (TPSPlayerController)
	{
		TPSPlayerController->SetHUDHealth(Health, MaxHealth);
	}

	if (HasAuthority())
	{
		//Damage입었을 때 호출할 함수 델리게이트에 연결
		OnTakeAnyDamage.AddDynamic(this, &ATPSCharacter::ReciveDamage);
	}
}

void ATPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
	HideCameraIfCharacterClose();
	PoolInit();
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

	//발사
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ATPSCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ATPSCharacter::FireButtonReleased);

	//장전
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ATPSCharacter::ReloadButtonPressed);
}

void ATPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//복제된 변수를 가진 클래스와 복제된 변수
	//모든 클라이언트에 복제됨
	//DOREPLIFETIME(ATPSCharacter, OverlappingWeapon);

	//무기에 Overlapping된 클라이언트와 서버에만 복제
	DOREPLIFETIME_CONDITION(ATPSCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ATPSCharacter, Health);
}

void ATPSCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}

//애니메이션 관련
void ATPSCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	UAnimInstance* AnimInstace = GetMesh()->GetAnimInstance();
	if (AnimInstace && FireWeaponMontage)
	{
		AnimInstace->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstace->Montage_JumpToSection(SectionName);
	}
}

void ATPSCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	UAnimInstance* AnimInstace = GetMesh()->GetAnimInstance();
	if (AnimInstace && HitReactMontage)
	{
		AnimInstace->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstace->Montage_JumpToSection(SectionName);
	}

}



///

//플레이어 조작 관련
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
//

// 무기 장착 관련
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

void ATPSCharacter::EquipButtonPressed()
{
	//무기 장착 버튼을 눌렀을 때 서버는 장비를 그냥 장착하고
	//클라이언트면 RPC호출
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



AWeapon* ATPSCharacter::GetEquippedWeapon()
{
	if(Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

void ATPSCharacter::ServerEquipButtomPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}
//재장전 애니메이션
void ATPSCharacter::PlayReloadMontage()
{
	UAnimInstance* AnimInstace = GetMesh()->GetAnimInstance();
	if (AnimInstace && ReloadMontage)
	{
		AnimInstace->Montage_Play(ReloadMontage);
		//장착된 무기에 따라 재생되는 모션 다르게
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		}
		AnimInstace->Montage_JumpToSection(SectionName);
	}
}



void ATPSCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	//무기에 겹친게 서버일 때
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ATPSCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}


//////
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
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}
bool ATPSCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

//전투 관련 입력 콜백

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

void ATPSCharacter::FireButtonPressed()
{
	
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(true);
	}
}

void ATPSCharacter::ReloadButtonPressed()
{
	if (Combat)
	{
		Combat->Reload();
	}
}

void ATPSCharacter::FireButtonReleased()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(false);
	}
}

float ATPSCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ATPSCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	//속도 계산
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	if (Speed == 0.0f && !bIsInAir) //Standing 상태이고 점프하고 있지 않을 때
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation =  FRotator(0.f, GetBaseAimRotation().Yaw, 0.0f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir) //running이거나 jump상태일때
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.0f);
		AO_Yaw = 0.0;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
	CalculateAO_Pitch();
}


//Proxy
void ATPSCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ATPSCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	UE_LOG(LogTemp, Warning, TEXT("ProxyYaw: %f"), ProxyYaw);

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ATPSCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

//플레이어 HP
void ATPSCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	if (!bEliminated)
	{
		PlayHitReactMontage();
	}
	
}

void ATPSCharacter::ReciveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	if(Health > 0.f)
	{
		PlayHitReactMontage();	
	}
	if (Health <= 0.f)
	{
		ATPSGameMode* TPSGameMode = GetWorld()->GetAuthGameMode<ATPSGameMode>();
		if (TPSGameMode)
		{
			TPSPlayerController = TPSPlayerController == nullptr ? Cast<ATPSPlayerController>(Controller) : TPSPlayerController;
			ATPSPlayerController* AttackerController = Cast<ATPSPlayerController>(InstigatorController);
			TPSGameMode->EliminatePlayer(this, TPSPlayerController, AttackerController);
		}
	}
	UpdateHUDHealth();
	
}

void ATPSCharacter::UpdateHUDHealth()
{
	TPSPlayerController = TPSPlayerController == nullptr ? Cast<ATPSPlayerController>(Controller) : TPSPlayerController;
	if (TPSPlayerController)
	{
		TPSPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}



//플레이어 제거(사망), 리스폰
void ATPSCharacter::MulticastEliminated_Implementation()
{
	if (TPSPlayerController)
	{
		TPSPlayerController->SetHUDWeaponAmmo(0);
	}
	bEliminated = true;
	PlayDeathMontage();

	//플레이어 움직임 비활성화
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (TPSPlayerController)
	{
		DisableInput(TPSPlayerController);
	}
	//충돌 무효화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATPSCharacter::PlayDeathMontage()
{
	UAnimInstance* AnimInstace = GetMesh()->GetAnimInstance();
	if (AnimInstace && DeathMontage)
	{
		AnimInstace->Montage_Play(DeathMontage);
	}
}
void ATPSCharacter::Eliminated()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	MulticastEliminated();
	GetWorldTimerManager().SetTimer
	(
		ElimTimer,
		this,
		&ATPSCharacter::ElimTimerFinishied,
		ElimDelay
	);
}

void ATPSCharacter::ElimTimerFinishied()
{
	ATPSGameMode* TPSGameMode = GetWorld()->GetAuthGameMode<ATPSGameMode>();
	if (TPSGameMode)
	{
		TPSGameMode->RequestRespawn(this, Controller);
	}
}


//전투  상태 리턴
ECombatState ATPSCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}