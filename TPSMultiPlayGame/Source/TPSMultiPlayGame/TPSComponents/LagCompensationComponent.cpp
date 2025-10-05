

#include "LagCompensationComponent.h"
#include "TPSMultiPlayGame/PlayerController/TPSPlayerController.h"
#include "TPSMultiPlayGame/Public/Character/TPSCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"

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
		ShowFramePackage(ThisFrame, FColor::Red);
	}
}


void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ATPSCharacter>(GetOwner()) : Character;
	if (Character)
	{
		//시간 저장
		Package.Time = GetWorld()->GetTimeSeconds();

		//그 시간의 HitBoxes들의 정보
		for (auto& BoxPair : Character->HitBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
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

		//같은 이름을 가진 히트 박스 정보를 Older와 Younger에서 가져오기
		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];

		//보간된 데이터 저장
		FBoxInformation InterpBoxInfo;

		//InterpFraction 시점의 위치와 회전 계산, Extent는 항상 고정되어 있으므로 보간될 필요가 없음
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

		//해당 부위 HitBox의 보간 정보를 InterpFramePackage의 HitBoxInfo(HitBox저장된 Map)에 추가
		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
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

void ULagCompensationComponent::ServerSideRewind(ATPSCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	bool bReturn =
		HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistroy.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistroy.GetTail() == nullptr;
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
		return;
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
		//Younger 와 Older사이를 보간
	}

	if (bReturn) return;
}




