// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TPSAnimInstance.h"
#include "Character/TPSCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "TPSMultiPlayGame/Weapon/Weapon.h"

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
	EquippedWeapon = TPSCharacter->GetEquippedWeapon();
	bIsCrouched = TPSCharacter->bIsCrouched;
	bIsAiming = TPSCharacter->IsAiming();
	TurningInPlace = TPSCharacter->GetTurningInPlace();
	bRotateRootBone = TPSCharacter->ShouldRotateRootBone();

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


	//AimOffset
	AO_Yaw = TPSCharacter->GetAO_Yaw();
	AO_Pitch = TPSCharacter->GetAO_Pitch();

	//IK
	//TODO : FABRIK 적용시 팔이 잘못된 각도로 꺾이는 현상 수정 필요
	if (bWeaponEquiped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && TPSCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		DrawDebugSphere(GetWorld(), LeftHandTransform.GetLocation(), 10.f, 12, FColor::Green, false, -1, 0, 1.f);

		//무기 관련 로테이션
		//조준하는 방향과 총구 일치하게
		if (TPSCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = TPSCharacter->GetMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - TPSCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
		

		FVector OutPosition;
		FRotator OutRotation;
		TPSCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}
}
