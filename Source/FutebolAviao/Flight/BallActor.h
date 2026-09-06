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

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Ball")
	UStaticMeshComponent* MeshComponent;

	FBallPhysics BallPhysics;
	FBallState BallState;
};
