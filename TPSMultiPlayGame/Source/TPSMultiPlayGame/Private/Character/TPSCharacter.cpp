// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TPSCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "TPSMultiPlayGame/Weapon/Weapon.h"
#include "Net/UnrealNetwork.h"
#include "TPSMultiPlayGame/TPSComponents/CombatComponent.h"
#include "TPSMultiPlayGame/TPSComponents/BuffComponent.h"
#include "TPSMultiPlayGame/TPSComponents/LagCompensationComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Character/TPSAnimInstance.h"
#include "TPSMultiPlayGame/TPSMultiPlayGame.h"
#include "TPSMultiPlayGame/PlayerController/TPSPlayerController.h"
#include "TPSMultiPlayGame/GameMode/TPSGameMode.h"
#include "TimerManager.h"
#include "TPSMultiPlayGame/PlayerState/TPSPlayerState.h"
#include "TPSMultiPlayGame/Weapon/WeaponTypes.h"
#include "Components/BoxComponent.h"


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

	//Combat컴포넌트
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	//Buff컴포넌트
	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	//LagCompensation 컴포넌트
	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));
	LagCompensation->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	//업데이트 빈도
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	//HitBoxes 초기화
	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	HitBoxes.Add(FName("head"), head);

	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitBoxes.Add(FName("pelvis"), pelvis);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitBoxes.Add(FName("spine_02"), spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitBoxes.Add(FName("spine_03"), spine_03);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitBoxes.Add(FName("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitBoxes.Add(FName("upperarm_r"), upperarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitBoxes.Add(FName("lowerarm_l"), lowerarm_l);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitBoxes.Add(FName("lowerarm_r"), lowerarm_r);

	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitBoxes.Add(FName("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitBoxes.Add(FName("hand_r"), hand_r);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitBoxes.Add(FName("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitBoxes.Add(FName("thigh_r"), thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitBoxes.Add(FName("calf_l"), calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitBoxes.Add(FName("calf_r"), calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitBoxes.Add(FName("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitBoxes.Add(FName("foot_r"), foot_r);

	
	for (auto Box : HitBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
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

	UpdateHUDShield();
	UpdateHUDHealth();

	TPSPlayerController = Cast<ATPSPlayerController>(Controller);
	if (TPSPlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(TPSPlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
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
	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PoolInit();
}

void ATPSCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
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
}


void ATPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	// 이동 (Axis2D)
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATPSCharacter::Move);

	// 둘러보기 (Axis2D)
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATPSCharacter::Look);

	// 점프 (Pressed)
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);

	// 장착 (Pressed)
	EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &ATPSCharacter::EquipButtonPressed);

	// 웅크리기 (Pressed / Released)
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ATPSCharacter::CrouchButtonPressed);
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ATPSCharacter::CrouchButtonReleased);

	// 조준 (Pressed / Released)
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ATPSCharacter::AimButtonPressed);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ATPSCharacter::AimButtonReleased);

	// 발사 (Pressed / Released)
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ATPSCharacter::FireButtonPressed);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ATPSCharacter::FireButtonReleased);

	// 재장전 (Pressed)
	EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ATPSCharacter::ReloadButtonPressed);
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
	DOREPLIFETIME(ATPSCharacter, Shield);
	DOREPLIFETIME(ATPSCharacter, bDisableGameplay);
}

void ATPSCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
	}
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ATPSPlayerController>(Controller);
		}
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
void ATPSCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (bDisableGameplay) return;
	if (Controller != nullptr)
	{
		// 전/후 이동
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(ForwardDirection, MovementVector.Y);

		// 좌/우 이동
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}
void ATPSCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}


void ATPSCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;
	Crouch();
}

void ATPSCharacter::CrouchButtonReleased()
{
	UnCrouch();
}

void ATPSCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ATPSCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ATPSCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(true);
	}
}

void ATPSCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;
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
void ATPSCharacter::Jump()
{
	if (bDisableGameplay) return;;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
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
	if (bDisableGameplay) return;
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
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_Pistol :
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
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

void ATPSCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

//플레이어 HP
void ATPSCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	if (!bEliminated && Health < LastHealth)
	{
		PlayHitReactMontage();
	}
	
}

void ATPSCharacter::ReciveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	//체력계산 - 실드 적용
	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{
			Shield = 0.f;
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
		}
	}
	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

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
	
}

void ATPSCharacter::UpdateHUDHealth()
{
	TPSPlayerController = TPSPlayerController == nullptr ? Cast<ATPSPlayerController>(Controller) : TPSPlayerController;
	if (TPSPlayerController)
	{
		TPSPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ATPSCharacter::UpdateHUDShield()
{
	TPSPlayerController = TPSPlayerController == nullptr ? Cast<ATPSPlayerController>(Controller) : TPSPlayerController;
	if (TPSPlayerController)
	{
		TPSPlayerController->SetHUDShield(Shield, MaxShield);
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
	//일부 조작 비활성화
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
	//충돌 무효화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//사망시 저격 조준경 제거
	bool bHideSniperScope = IsLocallyControlled() &&
		Combat &&
		Combat->bAiming &&
		Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;

	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}
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

bool ATPSCharacter::IsLocallyReloading()
{
	if(Combat == nullptr) return false;
	return Combat->bLocallyReloading;
}