#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

//Hit Box들을 저장하는 구조체
USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

//위치 정보를 저장하는 구조체
USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	ATPSCharacter* Character;
};

//Server-Side Rewind 불리언
USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
};

//샷건용 Server-Side Rewind 구조체
USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	//피격된 캐릭터들 정보
	UPROPERTY()
	TMap<ATPSCharacter*, uint32> HeadShots;

	UPROPERTY()
	TMap<ATPSCharacter*, uint32> BodyShots;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPSMULTIPLAYGAME_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	friend class ATPSCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, FColor Color);

/// <summary>
/// HitScan Server-Side Rewind 관련
/// </summary>
	UFUNCTION(Server, Reliable)
	void ServerScoreRequst(ATPSCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,const FVector_NetQuantize& HitLocation,float HitTime);
	FServerSideRewindResult ServerSideRewind(class ATPSCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime);
	//피격 확인
	FServerSideRewindResult ConfirmHit(const FFramePackage& Package, ATPSCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation);

/// <summary>
/// 샷건 관련 Server-Side Rewind
/// </summary>
	FShotgunServerSideRewindResult ShotgunServerSideRewind(const TArray<ATPSCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime);

	FShotgunServerSideRewindResult ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations);

	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(const TArray<ATPSCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, 
		const TArray<FVector_NetQuantize>& HitLocations, float HitTime);

/// <summary>
/// 발사체 Server-Side Rewind
/// </summary>
	FServerSideRewindResult ProjectileServerSideRewind(ATPSCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity, float HitTime);
	
	FServerSideRewindResult ProjectileConfirmHit(const FFramePackage& Package, ATPSCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime);

	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(
		ATPSCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);
	
protected:
	virtual void BeginPlay() override;
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage InterpBetweenFrams(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);

	

	//HitBox 캐시
	void CacheBoxPositions(ATPSCharacter* HitCharacter, FFramePackage& OutFramePackage);
	
	//HitBox이동
	void MoveBoxes(ATPSCharacter* HitCharacter, const FFramePackage& Package);

	//이동했던 HitBoxes 제자리로
	void ResetHitBoxes(ATPSCharacter* HitCharacter, const FFramePackage& Package);

	//캐릭터 메시의 충돌 활성화
	void EnableCharacterMeshCollision(ATPSCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);

	//Tick 에서 호출될 프레임 패키지 저장
	void SaveFramePackage();

	//피격된 시점의 FramePackage 정보
	FFramePackage GetFrameToCheck(ATPSCharacter* HitCharacter, float HitTime);


private:

	UPROPERTY()
	ATPSCharacter* Character;


	//FramePackage저장용
	TDoubleLinkedList<FFramePackage> FrameHistroy;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;


	UPROPERTY()
	class ATPSPlayerController* Controller;
	

		
};
