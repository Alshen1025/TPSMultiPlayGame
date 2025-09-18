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

protected:
	virtual void BeginPlay() override;


	void MoveFoward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAceess = "true"))
	class UWidgetComponent* OverheadWidget;
	

};
