

#include "LagCompensationComponent.h"
#include "TPSMultiPlayGame/PlayerController/TPSPlayerController.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "TPSMultiPlayGame/Weapon/Weapon.h"
#include "TPSMultiPlayGame/TPSMultiPlayGame.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}
void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SaveFramePackage();
}
void ULagCompensationComponent::SaveFramePackage()
{
	//Frame의 저장은 서버에서만 해야함
	if (Character == nullptr || !Character->HasAuthority()) return;
	if (FrameHistroy.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistroy.AddHead(ThisFrame);
	}
	else
	{	//									머리 노드(가장 최근 노드)      시간 값 가져오기 - 꼬리 노드(가장 구식 노드) 시간값 = FramPackage를 저장한 총 시간
		float HistoryLength = FrameHistroy.GetHead()->GetValue().Time - FrameHistroy.GetTail()->GetValue().Time;
		while (HistoryLength > MaxRecordTime) //최대 저장 시간보다 크면 노드 삭제
		{
			FrameHistroy.RemoveNode(FrameHistroy.GetTail());
			//노드 제거 후 시간 다시 계산
			HistoryLength = FrameHistroy.GetHead()->GetValue().Time - FrameHistroy.GetTail()->GetValue().Time;
		}
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistroy.AddHead(ThisFrame);

		//디버그 용 박스 그리기
		//ShowFramePackage(ThisFrame, FColor::Red);
	}
}
void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ATPSCharacter>(GetOwner()) : Character;
	if (Character)
	{
		//시간 저장
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;

		//그 시간의 HitBoxes들의 정보
		for (auto& BoxPair : Character->HitBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetUnscaledBoxExtent();
			if (BoxPair.Key == FName("head"))
			{
				UE_LOG(LogTemp, Warning, TEXT("SaveFramePackage: Character %s, Head Extent = %s"), *Character->GetName(), *BoxInformation.BoxExtent.ToString());
			}
			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}
FFramePackage ULagCompensationComponent::InterpBetweenFrams(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	//두 프레임 사이의 간격
	const float Distance = YoungerFrame.Time - OlderFrame.Time;

	//(HitTime - OlderFrame.Time) =  피격이 발생한 시간이 OlderFrame으로부터 얼마나 지난 시간인가
	//(HitTime - OlderFrame.Time) / Distance 두 프레임 사이 간격에서 피격시간의 상대적인 위치
	//0과 1사이로 고정
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

	//보간해서 만들어낼 가상 프레임 데이터 저장 변수
	FFramePackage InterpFramePackage;
	//이 가상 프레임의 시간은 피격시간으로 설정
	InterpFramePackage.Time = HitTime;

	//YoungerFrame의 HitBoxes 순회
	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		//현재 처리중인 HitBox의 이름
		const FName& BoxInfoName = YoungerPair.Key;
		const FBoxInformation& YoungerBox = YoungerPair.Value;

		//같은 이름을 가진 히트 박스 정보를 Older와 Younger에서 가져오기
		const FBoxInformation* OlderBoxPtr = OlderFrame.HitBoxInfo.Find(BoxInfoName);
		
		if (OlderBoxPtr)
		{
			const FBoxInformation& OlderBox = *OlderBoxPtr;
			//보간된 데이터 저장
			FBoxInformation InterpBoxInfo;
			//InterpFraction 시점의 위치와 회전 계산, Extent는 항상 고정되어 있으므로 보간될 필요가 없음
			InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
			InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
			InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;
			//해당 부위 HitBox의 보간 정보를 InterpFramePackage의 HitBoxInfo(HitBox저장된 Map)에 추가
			InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
		}
	}

	return InterpFramePackage;
}
FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ATPSCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();
	//현재 프레임의 패키지 정보를 저장
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	//피격 당시 시간대의 위치로 HitBox이동
	MoveBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	//머리(HeadShot)에 대한 충돌 먼저 활성화
	UBoxComponent* HeadBox = HitCharacter->HitBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	//피격 당시 시간대로 옮긴 Box에 대해서 라인 트래이스 실행
	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	UWorld* World = GetWorld();
	if (World)
	{
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_HitBox
		);
		if (ConfirmHitResult.bBlockingHit) //머리를 맞았을 경우를 가장 먼저 판정
		{
			if (ConfirmHitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
				if (Box)
				{
					DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
				}
			}
			//HitBox들 제자리로
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			//결과 (피격 true, 헤드샷 true 리턴)
			return FServerSideRewindResult{ true, true };
		}
		else //맞은게 머리가 아닌 경우에 나머지 박스에 대한 충돌 활성화 후 라인트레이스
		{
			for (auto& HitBoxPair : HitCharacter->HitBoxes)
			{
				if (HitBoxPair.Value != nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
				}
			}
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);
			if (ConfirmHitResult.bBlockingHit)
			{
				//Box원래대로
				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
				//결과
				return FServerSideRewindResult{ true, false };
			}
		}
	}
	//그 어디에도 피격되지 않음
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{ false, false };
}
FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	for (auto& Frame : FramePackages)
	{
		if (Frame.Character == nullptr) return FShotgunServerSideRewindResult();
	}
	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	for (auto& Frame : FramePackages)
	{
		if (Frame.Character == nullptr) continue;
		FFramePackage CurrentFrame;
		CurrentFrame.Character = Frame.Character;
		CacheBoxPositions(Frame.Character, CurrentFrame);
		MoveBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
		CurrentFrames.Add(CurrentFrame);
	}
	for (auto& Frame : FramePackages)
	{
		if (Frame.Character == nullptr) continue;
		//헤드 판정 먼저
		UBoxComponent* HeadBox = Frame.Character->HitBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}
	UWorld* World = GetWorld();
	//헤드샷 관련 라인트레이스
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		if (World)
		{
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);
			ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(ConfirmHitResult.GetActor());
			if (TPSCharacter)
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
					if (Box)
					{
						DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
					}
				}
				if (ShotgunResult.HeadShots.Contains(TPSCharacter))
				{
					ShotgunResult.HeadShots[TPSCharacter]++;
				}
				else
				{
					ShotgunResult.HeadShots.Emplace(TPSCharacter, 1);
				}
			}
		}
	}
	//헤드 이후에는 Body판정
	for (auto& Frame : FramePackages)
	{
		if (Frame.Character == nullptr) continue;
		for (auto& HitBoxPair : Frame.Character->HitBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				//Body박스 활성화
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}
		//HeadBox는 비활성화
		UBoxComponent* HeadBox = Frame.Character->HitBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	for (auto& HitLocation : HitLocations)
	{
		//Body에 대한 판정
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		if (World)
		{
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);
			ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(ConfirmHitResult.GetActor());
			if (TPSCharacter)
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
					if (Box)
					{
						DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
					}
				}
				if (ShotgunResult.BodyShots.Contains(TPSCharacter))
				{
					ShotgunResult.BodyShots[TPSCharacter]++;
				}
				else
				{
					ShotgunResult.BodyShots.Emplace(TPSCharacter, 1);
				}
			}
		}
	}
	//모든 박스 원위치
	for (auto& Frame : CurrentFrames)
	{
		if (Frame.Character == nullptr) continue;
		ResetHitBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
	}
	return ShotgunResult;
}
void ULagCompensationComponent::CacheBoxPositions(ATPSCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr) return;
	//HitBox순회
	for (auto& HitBoxPair : HitCharacter->HitBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetUnscaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}
void ULagCompensationComponent::MoveBoxes(ATPSCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;
	for (auto& HitBoxPair : HitCharacter->HitBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			// Find()를 사용해 안전하게 데이터 가져오기
			//Package의 값으로 현재 캐릭터의 HitBox들 이동
			const FBoxInformation* BoxInfo = Package.HitBoxInfo.Find(HitBoxPair.Key);
			if (BoxInfo)
			{
				HitBoxPair.Value->SetWorldLocation(BoxInfo->Location);
				HitBoxPair.Value->SetWorldRotation(BoxInfo->Rotation);
				HitBoxPair.Value->SetBoxExtent(BoxInfo->BoxExtent);
			}
		}
	}
}
void ULagCompensationComponent::ResetHitBoxes(ATPSCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;
	for (auto& HitBoxPair : HitCharacter->HitBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			const FBoxInformation* BoxInfo = Package.HitBoxInfo.Find(HitBoxPair.Key);
			if (BoxInfo)
			{
				
				HitBoxPair.Value->SetWorldLocation(BoxInfo->Location);
				HitBoxPair.Value->SetWorldRotation(BoxInfo->Rotation);
				HitBoxPair.Value->SetBoxExtent(BoxInfo->BoxExtent);
			}
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}
void ULagCompensationComponent::EnableCharacterMeshCollision(ATPSCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}
void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, FColor Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			//디버그 4초동안만 보여주기
			false,
			4.f
		);
	}
}
FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ATPSCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	//Server-Side Rewind를 이용한 피격 판정 진행 후 결과 값 리턴
	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}
FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<ATPSCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	//피격된 캐릭터들에게서 판정할 히트박스 정보 가져와서 리턴
	for (ATPSCharacter* HitCharacter : HitCharacters)
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}
	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}
FFramePackage ULagCompensationComponent::GetFrameToCheck(ATPSCharacter* HitCharacter, float HitTime)
{
	bool bReturn =
		HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistroy.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistroy.GetTail() == nullptr;
	if (bReturn) return FFramePackage();
	//피격 되었는지 확인 해야하는 프레임의 FramePackage
	FFramePackage FrameToCheck;

	bool bShouldInterpolate = true;
	// 피격된 캐릭터의 FrameHistroy에 접근
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistroy;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	if (OldestHistoryTime > HitTime)
	{
		//프레임이 기록된 시간보다 더 이전에 피격됨.
		//피격 판정 불가
		return FFramePackage();
	}
	if (OldestHistoryTime == HitTime)
	{
		//맨 마지막 프레임 패키지의 시간과 피격 시간 일치
		//맨 마지막 프레임 패키지가 확인 해야하는 프레임
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	if (NewestHistoryTime <= HitTime)
	{
		//가장 최근 프레임 패키지 시간과 피격 시간 일치
		//가장 최근 프레임 패키지가 확인 해야하는 프레임
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	//확인해야하는 프레임 패키지가 리스트 맨 마지막과 처음이 아니라면
	//HitTime이후 프레임 패키지
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	//HitTime이전 프레임 패키지
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
	while (Older->GetValue().Time > HitTime) //Older가 HitTime보다 최근이면
	{
		//OlderTime < HitTime < YoungerTime가 될 때까지 루프
		//Head노드가 가장 최근에 추가된 노드임

		//다음 노드로 이동
		if (Older->GetNextNode() == nullptr) break;
		Older = Older->GetNextNode();
		//HitTime과 비교하고 여전히 Older가 HitTime보다 최근
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}
	//루프를 통해 찾은 Older프레임 시간이 정확히 일치하는 경우
	if (Older->GetValue().Time == HitTime)
	{
		FrameToCheck = Older->GetValue();
		//두 프레임 페키지 사이의 보간이 필요가 없음
		bShouldInterpolate = false;
	}
	if (bShouldInterpolate)
	{
		//Younger 와 Older사이를 보간해야함
		FrameToCheck = InterpBetweenFrams(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	FrameToCheck.Character = HitCharacter;
	return FrameToCheck;
}
void ULagCompensationComponent::ServerScoreRequst_Implementation(ATPSCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	//ServerSideRewind 호출해서 피격 판정 결과 얻기
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	if (Character && HitCharacter && Character->GetEquippedWeapon() && Confirm.bHitConfirmed)
	{
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();

		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Damage,
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}
void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<ATPSCharacter*>& HitCharacters,
	const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	UE_LOG(LogTemp, Warning, TEXT("--- [SERVER DEBUG] ShotgunServerScoreRequest RPC Received ---"));
	UE_LOG(LogTemp, Warning, TEXT("[SERVER DEBUG] Number of potential targets: %d"), HitCharacters.Num());

	//피격 판정
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	// 피격 판정 결과를 상세히 로그로 출력.
	if (Confirm.HeadShots.Num() > 0)
	{
		for (auto& HeadShotPair : Confirm.HeadShots)
		{
			if (HeadShotPair.Key)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SERVER DEBUG] Rewind Result -> HEADSHOT on %s, Pellet Count: %d"), *HeadShotPair.Key->GetName(), HeadShotPair.Value);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SERVER DEBUG] Rewind Result -> No headshots confirmed."));
	}

	if (Confirm.BodyShots.Num() > 0)
	{
		for (auto& BodyShotPair : Confirm.BodyShots)
		{
			if (BodyShotPair.Key)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SERVER DEBUG] Rewind Result -> BODYSHOT on %s, Pellet Count: %d"), *BodyShotPair.Key->GetName(), BodyShotPair.Value);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SERVER DEBUG] Rewind Result -> No bodyshots confirmed."));
	}


	for (auto& HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || Character == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("[SERVER DEBUG] Damage application skipped for one character due to nullptr."));
			continue;
		}

		//데미지 합산 용
		float TotalDamage = 0.f;

		//HeadShot처리
		if (Confirm.HeadShots.Contains(HitCharacter))
		{
			float HeadShotDamage = Confirm.HeadShots[HitCharacter] * HitCharacter->GetEquippedWeapon()->GetHeadShotDamage();
			TotalDamage += HeadShotDamage;
			UE_LOG(LogTemp, Warning, TEXT("[SERVER DEBUG] Calculated Headshot Damage for %s: %.2f"), *HitCharacter->GetName(), HeadShotDamage);
		}

		//BodyShot처리
		if (Confirm.BodyShots.Contains(HitCharacter))
		{
			float BodyShotDamage = Confirm.BodyShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage();
			TotalDamage += BodyShotDamage;
			UE_LOG(LogTemp, Warning, TEXT("[SERVER DEBUG] Calculated Bodyshot Damage for %s: %.2f"), *HitCharacter->GetName(), BodyShotDamage);
		}

		if (TotalDamage > 0.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SERVER DEBUG] Applying Total Damage %.2f to %s"), TotalDamage, *HitCharacter->GetName());
			//합산한 데미지 적용
			UGameplayStatics::ApplyDamage(
				HitCharacter,
				TotalDamage,
				Character->Controller,
				Character->GetEquippedWeapon(),
				UDamageType::StaticClass()
			);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[SERVER DEBUG] Total damage for %s is 0. Damage not applied."), *HitCharacter->GetName());
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("--- [SERVER DEBUG] ShotgunServerScoreRequest Finished ---\n"));
}
//Projectile ServerSideRewind 함수
FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ATPSCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}
FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, ATPSCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	//현재 프레임 정보 저장
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	//헤드 먼저 충돌 활성화
	UBoxComponent* HeadBox = HitCharacter->HitBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	//PredictProjectilePath이용
	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithCollision = true;
	PathParams.MaxSimTime = MaxRecordTime;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.TraceChannel = ECC_HitBox;
	PathParams.ActorsToIgnore.Add(GetOwner());

	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	if (PathResult.HitResult.bBlockingHit) //머리에 피격되었다면
	{
		ResetHitBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{ true, true };
	}

	else //머리에 피격이 아니면 다른 부위 활성화 후 검사
	{
		for (auto& HitBoxPair : HitCharacter->HitBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}

		UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
		if (PathResult.HitResult.bBlockingHit)
		{
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{ true, false };
		}
	}

	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{ false, false };
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ATPSCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);

	if (Character && HitCharacter && Confirm.bHitConfirmed && Character->GetEquippedWeapon())
	{
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();

		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Damage,
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}
