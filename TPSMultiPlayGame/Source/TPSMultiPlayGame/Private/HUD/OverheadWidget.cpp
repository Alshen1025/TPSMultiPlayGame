// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/OverheadWidget.h"
#include "Components/TextBlock.h"
#include "TPSMultiPlayGame/PlayerState/TPSPlayerState.h"

void UOverheadWidget::SetDisplayText(FString TextToDispaly)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDispaly));
	}
}

void UOverheadWidget::SetPlayerName(FString PlayerName)
{
   SetDisplayText(PlayerName);
}

void UOverheadWidget::SetDisplayColor(const FLinearColor& Color)
{
    DisplayText->SetColorAndOpacity(FSlateColor(Color));
}