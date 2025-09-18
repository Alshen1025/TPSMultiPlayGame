// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TPSCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"

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
}

void ATPSCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ATPSCharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &ATPSCharacter::MoveFoward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATPSCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ATPSCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ATPSCharacter::LookUp);
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


