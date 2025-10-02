// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"


UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class TPSMULTIPLAYGAME_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	void ShowPickupWidget(bool bShowWidget);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;
	virtual void Fire(const FVector& HitTarget);

	//장착한 플레이어 사망 시 무기 드랍
	void Dropped();

	//라인 트레이스 지점에 구를 만들고 그 안에서 몇 개의 지점 고르기
	FVector TraceEndWithScatter(const FVector& HitTarget);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	/**
	*탄착군 관련 변수들
	*/
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	//줌 관련 변수
	//줌 FOV
	UPROPERTY(EditAnywhere)
	float ZoomFOV = 30.f;
	//줌 속도
	UPROPERTY(EditAnywhere)
	float ZoominterpSpeed = 20.f;

	//장탄수 관련 변수
	//가지고 있는 총 탄약 수
	UPROPERTY(EditAnywhere)
	int32 Ammo;

	//처리되지 않은 서버의 요청 수 - > ClientAmmoUpdate에서 사용
	//Client-Side Predicting
	int32 Sequence = 0;

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);


	void SpendRound();
	//음수 넣으면 개수 차감도 가능
	


	//탄창용량
	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	UPROPERTY()
	class ATPSCharacter* TPSOwnerCharacter;

	UPROPERTY()
	class ATPSPlayerController* TPSOwnerController;

	//무기 타입
	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

public:
	//무기 타입 Getter
	FORCEINLINE EWeaponType GetWeaponType() const {  return WeaponType;  }
	void SetHUDAmmo();

	//탄약 잔량 확인
	bool IsEmpty();
	bool IsFull();

	//사운드
	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	UPROPERTY(EditAnywhere)
	//무기의 발사 유형
	EFireType FireType;

public:	

	//조준선을 위한 텍스쳐 변수
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsBottom;
	
	virtual void Tick(float DeltaTime) override;
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	FORCEINLINE float GetZoomInterpSpeed() const { return ZoominterpSpeed; }

	FORCEINLINE float GetZoomedFOV() const { return ZoomFOV; }

	//연사 기능
	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = .15f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;


	//탄약 관련 Getter
	//장전된 탄약 용량
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	//탄창 용량
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	//탄약 더하기
	void AddAmmo(int32 AmmoToAdd);

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	

	
};
