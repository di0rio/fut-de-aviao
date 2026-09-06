#pragma once

#include "PureMath.h"

struct FBallPhysicsParams
{
	float Gravity = 980.f;          // cm/s^2, puxando pra baixo
	float Drag = 0.35f;             // fracao da velocidade perdida por segundo
	float Restitution = 0.75f;      // quanto da velocidade sobra depois de quicar
	float Radius = 150.f;
	float MaxSpeed = 12000.f;
	float HitTransfer = 1.6f;   // quanto da velocidade de aproximacao do aviao vira impulso
	float MinKick = 500.f;      // toque de raspao ainda mexe com a bola

	// Arena: caixa fechada. X e Y sao metades; o chao e Z=0.
	float ArenaHalfX = 10000.f;
	float ArenaHalfY = 6000.f;
	float ArenaCeilingZ = 5000.f;
};

struct FBallState
{
	PureMath::FPureVector Position;
	PureMath::FPureVector Velocity;
};

class FBallPhysics
{
public:
	FBallPhysics() : Params() {}
	explicit FBallPhysics(const FBallPhysicsParams& InParams) : Params(InParams) {}

	void Update(FBallState& State, float DeltaSeconds) const;

	// True quando as esferas de aviao e bola se tocam.
	bool IsOverlapping(const FBallState& State, const PureMath::FPureVector& PlanePosition, float PlaneRadius) const;

	// Impulso do aviao na bola, mais o empurrao que desfaz a sobreposicao -- sem
	// ele o aviao "carrega" a bola, reacertando todo frame.
	void ApplyHit(FBallState& State, const PureMath::FPureVector& PlanePosition, const PureMath::FPureVector& PlaneVelocity, float PlaneRadius) const;

	const FBallPhysicsParams& GetParams() const { return Params; }

private:
	FBallPhysicsParams Params;
};
