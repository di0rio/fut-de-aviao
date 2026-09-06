#pragma once

#include "PureMath.h"
#include "ArenaGeometry.h"

struct FBallPhysicsParams
{
	float Gravity = 980.f;          // cm/s^2, puxando pra baixo
	float Drag = 0.20f;             // fracao da velocidade perdida por segundo
	float Restitution = 0.75f;      // quanto da velocidade sobra depois de quicar
	float Radius = 400.f;           // 8m de diametro: visivel a 400m de distancia
	float MaxSpeed = 6000.f;        // 60 m/s: abaixo do boost (65), acima do cruzeiro (45)
	float HitTransfer = 1.1f;       // acerto em cruzeiro produz 5250, abaixo do teto
	float MinKick = 300.f;          // raspao ainda mexe na bola, sem catapultar

	// Fonte unica da geometria da arena, compartilhada com FMatchParams -- ver
	// ArenaGeometry.h.
	FArenaGeometry Arena;
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
	// ele o aviao "carrega" a bola, reacertando todo frame. Implementado como
	// ApplyImpulse seguido de PushOutOf (abaixo); mantido porque as suites de
	// teste chamam esta funcao combinada diretamente.
	void ApplyHit(FBallState& State, const PureMath::FPureVector& PlanePosition, const PureMath::FPureVector& PlaneVelocity, float PlaneRadius) const;

	// So a metade do impulso de ApplyHit: muda a velocidade, nao a posicao.
	// Existe separada para o caso de varios avioes sobrepondo a bola no mesmo
	// frame -- cada um contribui seu impulso antes de qualquer reposicionamento,
	// em vez de cada ApplyHit desfazer o empurrao do anterior.
	void ApplyImpulse(FBallState& State, const PureMath::FPureVector& PlanePosition, const PureMath::FPureVector& PlaneVelocity) const;

	// A outra metade de ApplyHit: so o empurrao que tira a bola de dentro da
	// esfera do aviao, sem tocar na velocidade. Chamada uma unica vez, mesmo
	// quando varios avioes se sobrepuseram e cada um ja recebeu seu
	// ApplyImpulse.
	void PushOutOf(FBallState& State, const PureMath::FPureVector& PlanePosition, const PureMath::FPureVector& PlaneVelocity, float PlaneRadius) const;

	const FBallPhysicsParams& GetParams() const { return Params; }
	void SetParams(const FBallPhysicsParams& InParams) { Params = InParams; }

private:
	FBallPhysicsParams Params;
};
