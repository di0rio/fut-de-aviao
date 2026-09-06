#pragma once

#include "PureMath.h"

struct FFlightPhysicsParams
{
	float Acceleration = 1950.f;  // 0 -> maxima em ~2.3s, mesmo tempo de antes
	float Deceleration = 1650.f;  // S freia de verdade: maxima -> parado em ~2.7s
	float Drag = 525.f;           // soltar o acelerador desacelera em ~6.5s
	float MaxSpeed = 4500.f;      // 45 m/s: atravessar o campo novo leva ~8.9s
	float BoostMaxSpeed = 6500.f; // 65 m/s: acima do teto da bola, pra poder alcanca-la
	float BoostAcceleration = 3750.f;
	float MinSpeed = 0.f;         // sem piso: o aviao so anda se voce acelerar
	float PitchRateDegPerSec = 110.f;
	float YawRateDegPerSec = 110.f;  // igual ao pitch: mirar em 3D fica simetrico
	float RollRateDegPerSec = 200.f;
	float MaxPitchDeg = 85.f;

	// Quao rapido o vetor velocidade persegue o nariz, por segundo. Valor alto =
	// velocidade cola no nariz (comportamento da Fase 2). Valor baixo = aviao
	// pesado, derrapa na curva. Em curva a taxa maxima (110 deg/s), o atraso de
	// regime fica em torno de 110/AlinhamentoPorSegundo graus -- com 25, isso da
	// um atraso de uns 4 graus. Valores bem altos (ex: 1000) saturam o blend e
	// desligam a inercia por completo.
	float VelocityAlignPerSec = 25.f;
};

struct FFlightPhysicsState
{
	PureMath::FPureVector Velocity;
	float PitchDeg = 0.f;
	float YawDeg = 0.f;
	float RollDeg = 0.f;
};

class FFlightPhysics
{
public:
	// UHT's generated vtable-helper constructor for APlanePawn needs this member default-constructible.
	FFlightPhysics() : Params() {}
	explicit FFlightPhysics(const FFlightPhysicsParams& InParams) : Params(InParams) {}

	// ThrottleInput, PitchInput, YawInput, RollInput sao esperados no intervalo [-1, 1].
	void Update(FFlightPhysicsState& State, float ThrottleInput, float PitchInput, float YawInput, float RollInput, bool bBoostActive, float DeltaSeconds) const;

	// Velocidade escalar. O estado guarda vetor; isto e so a magnitude.
	static float GetSpeed(const FFlightPhysicsState& State);

private:
	FFlightPhysicsParams Params;
};
