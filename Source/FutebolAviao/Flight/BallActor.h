#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BallPhysics.h"
#include "BallActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class FUTEBOLAVIAO_API ABallActor : public AActor
{
	GENERATED_BODY()

public:
	ABallActor();

	// Volta pro centro da arena, parada. Chamado no inicio e depois de cada gol.
	void ResetToCenter();

	const FBallState& GetBallState() const { return BallState; }

	// So leitura -- usado por APlanePawn pra checar em runtime se a regra de
	// design "cruzeiro < bola < boost" ainda vale depois de CVars de tuning
	// (ver ApplyTuningCVars em PlanePawn.cpp).
	const FBallPhysicsParams& GetBallParams() const { return BallPhysics.GetParams(); }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyTuningCVars();

	UPROPERTY(VisibleAnywhere, Category = "Ball")
	UStaticMeshComponent* MeshComponent;

	FBallPhysics BallPhysics;
	FBallState BallState;
};
