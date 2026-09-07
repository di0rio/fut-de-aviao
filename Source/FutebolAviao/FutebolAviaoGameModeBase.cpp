#include "FutebolAviaoGameModeBase.h"
#include "Flight/PlanePawn.h"
#include "Flight/BallActor.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"

AFutebolAviaoGameModeBase::AFutebolAviaoGameModeBase()
{
	DefaultPawnClass = APlanePawn::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void AFutebolAviaoGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ABallActor> It(GetWorld()); It; ++It)
	{
		Ball = *It;
		break;
	}

	if (!Ball)
	{
		Ball = GetWorld()->SpawnActor<ABallActor>(ABallActor::StaticClass(), FVector(0.f, 0.f, 1500.f), FRotator::ZeroRotator);
	}
}

void AFutebolAviaoGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Ball)
	{
		return;
	}

	const EGoalSide Side = MatchRules.CheckGoal(Ball->GetBallState().Position);
	if (Side != EGoalSide::None)
	{
		MatchRules.RegisterGoal(MatchState, Side);
		Ball->ResetToCenter();

		// Reset de verdade no gol: todo aviao volta pro proprio spawn, parado
		// e com tanque cheio, nao so a bola. FMatchRules de proposito nao sabe
		// nada disto -- reset e comportamento de adapter, nao regra pura de
		// partida.
		for (TActorIterator<APlanePawn> It(GetWorld()); It; ++It)
		{
			if (APlanePawn* Plane = *It)
			{
				Plane->ResetToSpawn();
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 0.05f, FColor::Cyan,
			FString::Printf(TEXT("Oeste %d  x  %d Leste"), MatchState.WestScore, MatchState.EastScore));
	}
#endif
}
