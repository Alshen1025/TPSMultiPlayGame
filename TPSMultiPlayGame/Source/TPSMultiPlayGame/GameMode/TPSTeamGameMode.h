#pragma once

#include "CoreMinimal.h"
#include "TPSGameMode.h"
#include "TPSTeamGameMode.generated.h"


/**
 * 
 */
UCLASS()
class TPSMULTIPLAYGAME_API ATPSTeamGameMode : public ATPSGameMode
{
	GENERATED_BODY()
	

public:
	ATPSTeamGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;

	virtual void EliminatePlayer(class ATPSCharacter* EliminatedCharacter, class ATPSPlayerController* VictimController, ATPSPlayerController* AttackerController) override;
protected:
	virtual void HandleMatchHasStarted() override;

};
