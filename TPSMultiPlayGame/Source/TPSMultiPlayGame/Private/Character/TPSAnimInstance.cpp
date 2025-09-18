// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TPSAnimInstance.h"
#include "Character/TPSCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UTPSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	TPSCharacter = Cast<ATPSCharacter>(TryGetPawnOwner());
}

void UTPSAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (TPSCharacter == nullptr)
	{
		TPSCharacter = Cast<ATPSCharacter>(TryGetPawnOwner());
	}
	if (TPSCharacter == nullptr) return;

	FVector Velocity = TPSCharacter->GetVelocity();
	Velocity.Z = 0.0f;
	Speed = Velocity.Size();

	bIsInAir = TPSCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = TPSCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquiped = TPSCharacter->IsWeaponEquipped();
	bIsCrouched = TPSCharacter->bIsCrouched;
	bIsAiming = TPSCharacter->IsAiming();

	// Offset Yaw for Strafing
	FRotator AimRotation = TPSCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(TPSCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;
	

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = TPSCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);
}
