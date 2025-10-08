// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayerController.h"
#include "TPSMultiPlayGame/HUD/TPSHUD.h"
#include "TPSMultiPlayGame/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "Net/UnrealNetwork.h"
#include "TPSMultiPlayGame/GameMode/TPSGameMode.h"
#include "TPSMultiPlayGame/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "TPSMultiPlayGame/TPSComponents/CombatComponent.h"
#include "TPSMultiPlayGame/GameState/TPSGameState.h"
#include "TPSMultiPlayGame/PlayerState/TPSPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Components/Image.h"
#include "TPSMultiPlayGame/HUD/ReturnToMainMenu.h"

void ATPSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	TPSHUD = Cast<ATPSHUD>(GetHUD());
	ServerCheckMatchState();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}
void ATPSPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
	CheckPing(DeltaTime);
	
}
void ATPSPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		if (PlayerState == nullptr)
		{
			PlayerState = GetPlayerState<APlayerState>();
		}
		if (PlayerState)
		{
			//1/4로 압축된 핑이기 때문에 *4를 해야 원래 핑이 됨
			if (PlayerState->GetCompressedPing() * 4 > HighPingThreshold)
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
	}
	bool bHighPingAnimationPlaying =
		TPSHUD && TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->HighPingAnimation &&
		TPSHUD->CharacterOverlay->IsAnimationPlaying(TPSHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}
//핑이 너무 높으면
void ATPSPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}
void ATPSPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATPSPlayerController, MatchState);
}
void ATPSPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
	
}
void ATPSPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (TPSHUD && TPSHUD->CharacterOverlay)
		{
			CharacterOverlay = TPSHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
			}
		}
	}
}
void ATPSPlayerController::SetHUDTime()
{
	//매치 상태에 따라 남은시간이 다름
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if(MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	//남은 시간 = MatchTIme(설정한 게임 시간) - 게임이 시작되고 흐른 시간
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
}



void ATPSPlayerController::HighPingWarning()
{
	//HighPing 이미지와 애니메이션 표시
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;

	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->HighPingImage &&
		TPSHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		TPSHUD->CharacterOverlay->HighPingImage->SetOpacity(1.0f);
		TPSHUD->CharacterOverlay->PlayAnimation(TPSHUD->CharacterOverlay->HighPingAnimation, 0.f, 5);
	}
}

void ATPSPlayerController::StopHighPingWarning()
{
	//HighPing 이미지와 애니메이션 숨기기
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;

	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->HighPingImage &&
		TPSHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		TPSHUD->CharacterOverlay->HighPingImage->SetOpacity(0.0f);
		if (TPSHUD->CharacterOverlay->IsAnimationPlaying(TPSHUD->CharacterOverlay->HighPingAnimation))
		{
			//애니메이션 정지
			TPSHUD->CharacterOverlay->StopAnimation(TPSHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

float ATPSPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds(); //서버면 그냥 현재 시간 쓰면 됨
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ATPSPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ATPSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr) return;

	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent)
	{
		if (Quit)
		{
			EnhancedInputComponent->BindAction(Quit, ETriggerEvent::Started, this, &ATPSPlayerController::ShowReturnToMainMunu);
		}
	}
}
void ATPSPlayerController::ShowReturnToMainMunu(const FInputActionValue& Value)
{
	if (ReturnToMainMenuWidget == nullptr) return;
	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

void ATPSPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceiced)
{
	//요청을 보내고 답을 받을 때 까지 걸린 시간(서버 - 클라이언트 왕복시간)
	//클라이언트가 서버에 요청을 보내고 얼마나 많은 시간이 흘렀는가
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5 * RoundTripTime;
	//서버의 현재 시간을 계산
	//서버가 클라이언트의 요청을 받은시간에 왕복 시간의 절반을 더하기
	float  CurrentSeverTime = TimeServerReceiced + (0.5 * RoundTripTime);

	//서버의 현재 시간에서 클라이언트의 현재 시간을 빼면 서버와 클라이언트 사이의 시간차이 계산 가능
	ClientServerDelta = CurrentSeverTime - GetWorld()->GetTimeSeconds();
}

//서버와 클라이언트 서버 시간 동기화
void ATPSPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	//서버는 자신의 현재 시간을 가져옴
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	//클라이언트 RPC호출해 서버 시간 전달
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

//게임 Match State
void  ATPSPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void  ATPSPlayerController::HandleMatchHasStarted()
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	if (TPSHUD && TPSHUD->CharacterOverlay == nullptr)
	{
		TPSHUD->AddCharacterOverlay();
		if (TPSHUD->Announcement && TPSHUD->Announcement->IsInViewport())
		{
			TPSHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

//매치가 종료되면
void ATPSPlayerController::HandleCooldown()
{
	
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	if (TPSHUD)
	{
		//캐릭터 오버레이 제거
		TPSHUD->CharacterOverlay->RemoveFromParent();
		if (TPSHUD->Announcement && TPSHUD->Announcement->AnnouncementText)
		{
			//Announcement 다시 생성 -> 남은 Cooldown시간 안내를 위해
			TPSHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			//Text변경
			FString AnnounceText("New Match Starts In:");
			TPSHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnounceText));

			ATPSGameState* TPSGameState = Cast<ATPSGameState>(UGameplayStatics::GetGameState(this));
			ATPSPlayerState* TPSPlayerState = GetPlayerState<ATPSPlayerState>();

			if (TPSGameState && TPSPlayerState)
			{
				TArray<ATPSPlayerState*> TopPlayers = TPSGameState->TopScoringPlayers;
				FString InfoText;
				if (TopPlayers.Num() == 0)
				{
					InfoText = FString("Draw...");
				}
				//최고 점수자가 1명이고 그게 본인일 때
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == TPSPlayerState)
				{
					InfoText = FString("You are the champion!");
				}
				//승자가 본인이 아닐 때
				else if (TopPlayers.Num() == 1)
				{
					InfoText = FString::Printf(TEXT("Champion \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				//승자가 여러명 일 때
				else if (TopPlayers.Num() > 1)
				{
					InfoText = FString("Players tied for the win:\n");
					//TopPlayers 순회
					for (auto TiedPlayer : TopPlayers)
					{
						InfoText.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
				TPSHUD->Announcement->InfoText->SetText(FText::FromString(InfoText));
			}

			
		}
	}
	ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(GetPawn());
	if (TPSCharacter && TPSCharacter->GetCombat())
	{
		TPSCharacter->bDisableGameplay = true;
		//게임이 종료 되자마자 발사 중단
		TPSCharacter->GetCombat()->FireButtonPressed(false);
	}
}
void ATPSPlayerController::OnRep_MatchState()
{
	
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
	
	
}

void ATPSPlayerController::ServerCheckMatchState_Implementation()
{
	ATPSGameMode* Gamemode = Cast<ATPSGameMode>(UGameplayStatics::GetGameMode(this));
	if (Gamemode)
	{
		WarmupTime = Gamemode->WarmupTime;
		MatchTime = Gamemode->MatchTime;
		LevelStartingTime = Gamemode->LevelStartingTime;
		CooldownTime = Gamemode->CooldownTime;
		MatchState = Gamemode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, LevelStartingTime, CooldownTime);
	}
}
void ATPSPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float Match, float StartingTime, float Cooldown)
{
	MatchState = StateOfMatch;
	WarmupTime = Warmup;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchTime = Match;
	OnMatchStateSet(MatchState);
	if (TPSHUD && MatchState == MatchState::WaitingToStart)
	{
		TPSHUD->AddAnnouncement();
	}
}
//
void ATPSPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;

	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->HealthBar &&
		TPSHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		TPSHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		TPSHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ATPSPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;

	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->ShieldBar &&
		TPSHUD->CharacterOverlay->ShieldText;
	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		TPSHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		TPSHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDHealth = Shield;
		HUDMaxHealth = MaxShield;
	}
}

void ATPSPlayerController::SetHUDScore(float Score)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		TPSHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}

}

void ATPSPlayerController::SetHUDDefeats(int32 Defeats)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		TPSHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ATPSPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		TPSHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ATPSPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
		TPSHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ATPSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(InPawn);
	if (TPSCharacter)
	{
		SetHUDHealth(TPSCharacter->GetHealth(), TPSCharacter->GetMaxHealth());
	}
}

void ATPSPlayerController::SetHUDMatchCountdown(float CountDownTime)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	bool bHUDValid = TPSHUD &&
		TPSHUD->CharacterOverlay &&
		TPSHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		if (CountDownTime < 0.f)
		{
			TPSHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountDownTime / 60.f);
		int32 Seconds = CountDownTime - Minutes * 60;


		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		TPSHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}

}

void ATPSPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	TPSHUD = TPSHUD == nullptr ? Cast<ATPSHUD>(GetHUD()) : TPSHUD;
	bool bHUDValid = TPSHUD &&
		TPSHUD->Announcement &&
		TPSHUD->Announcement->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			TPSHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;


		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		TPSHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}
