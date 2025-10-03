

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
	FFramPackage Package;
	SaveFramePackage(Package);
	ShowFramePackage(Package, FColor::Orange);
}
void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void ULagCompensationComponent::SaveFramePackage(FFramPackage& Package)
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
void ULagCompensationComponent::ShowFramePackage(const FFramPackage& Package, FColor Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			true
		);
	}
}




