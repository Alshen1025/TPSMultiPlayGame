// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

/**
 * 
 */
UCLASS()
class TPSMULTIPLAYGAME_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()
	

public:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DisplayText;

	void SetDisplayText(FString TextToDispaly);

	UFUNCTION(BlueprintCallable)
	void SetPlayerNetRole(APawn* InPawn);

protected:
	virtual void NativeDestruct() override;
};
