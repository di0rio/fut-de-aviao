#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FlightPhysics.h"
#include "PlanePawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class FUTEBOLAVIAO_API APlanePawn : public APawn
{
	GENERATED_BODY()

public:
	APlanePawn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void HandleThrottleInput(float Value);
	void HandlePitchInput(float Value);
	void HandleYawInput(float Value);
	void HandleRollInput(float Value);

private:
	UPROPERTY(VisibleAnywhere, Category = "Plane")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Plane")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, Category = "Plane")
	UCameraComponent* CameraComponent;

	FFlightPhysics FlightPhysics;
	FFlightPhysicsState FlightState;

	float ThrottleInput = 0.f;
	float PitchInput = 0.f;
	float YawInput = 0.f;
	float RollInput = 0.f;
};
