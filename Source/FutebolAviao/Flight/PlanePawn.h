#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FlightPhysics.h"
#include "FuelSystem.h"
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
	void HandleBoostPressed();
	void HandleBoostReleased();

public:
	// A Fase 4 (2v2) vai querer que o GameMode escolha o spawn de cada time.
	// Por enquanto o proprio BeginPlay define; isto e o gancho pra isso mudar.
	UFUNCTION(BlueprintCallable, Category = "Plane")
	void SetSpawnTransform(const FTransform& NewSpawnTransform);

	// A bola precisa saber onde o aviao esta e quao rapido vai, pra resolver o
	// impacto. Raio esferico aproximado do aviao, generoso de proposito: acertar
	// a bola tem que ser divertido, nao preciso.
	UFUNCTION(BlueprintCallable, Category = "Plane")
	float GetCollisionRadius() const { return CollisionRadius; }

	PureMath::FPureVector GetPureVelocity() const { return FlightState.Velocity; }

	// A regra de verdade e "destruido, aguardando respawn" -- esconder o ator
	// (SetActorHiddenInGame) e so o efeito colateral de hoje dessa regra, nao a
	// regra em si. Qualquer outra coisa que esconda o aviao no futuro
	// (espectador, cinematica, relevancia de rede) nao deve, por si so, tirar
	// o aviao da jogada.
	UFUNCTION(BlueprintCallable, Category = "Plane")
	bool IsFlying() const { return !FuelState.bIsDestroyed; }

private:
	// Zera o FlightState e o reancora na posicao e rotacao de Transform, usado
	// tanto no spawn inicial quanto no respawn.
	void ResetFlightStateTo(const FTransform& Transform);

	void ApplyTuningCVars();

	// Um unico lugar define o default; ApplyTuningCVars restaura a partir dele
	// todo tick, pra que voltar a CVar pra -1 volte de fato ao default.
	static constexpr float DefaultCollisionRadius = 600.f;
	float CollisionRadius = DefaultCollisionRadius;

	UPROPERTY(VisibleAnywhere, Category = "Plane")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Plane")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, Category = "Plane")
	UCameraComponent* CameraComponent;

	FFlightPhysics FlightPhysics;
	FFlightPhysicsState FlightState;

	FFuelSystem FuelSystem;
	FFuelState FuelState;

	// Transform pra onde o aviao volta ao respawnar. Capturado no BeginPlay por
	// padrao, mas so a posicao e rotacao sao restauradas via ResetFlightStateTo -
	// nao e um SetActorTransform completo. SetSpawnTransform deixa o GameMode
	// substituir isto (spawn de time, Fase 4).
	FTransform SpawnTransform;

	bool bBoostInput = false;
	bool bWasDestroyed = false;

	float ThrottleInput = 0.f;
	float PitchInput = 0.f;
	float YawInput = 0.f;
	float RollInput = 0.f;
};
