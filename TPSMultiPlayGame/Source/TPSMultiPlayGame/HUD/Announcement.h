// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Announcement.generated.h"

/**
 * 
 */
UCLASS()
class TPSMULTIPLAYGAME_API UAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY()
	class UTextBlock* WarmupTime;

	UPROPERTY()
	UTextBlock* AnnouncementText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* InfoText;
	
};
