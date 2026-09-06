#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Flight/MatchRules.h"
#include "FutebolAviaoGameModeBase.generated.h"

UCLASS()
class FUTEBOLAVIAO_API AFutebolAviaoGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFutebolAviaoGameModeBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	FMatchRules MatchRules;
	FMatchState MatchState;

	UPROPERTY()
	class ABallActor* Ball = nullptr;
};
