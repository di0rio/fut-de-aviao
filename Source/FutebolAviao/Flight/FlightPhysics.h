#pragma once

struct FFlightPhysicsParams
{
	float Acceleration = 2600.f;  // 0 -> maxima em ~2.3s
	float Deceleration = 2200.f;  // S freia de verdade: maxima -> parado em ~2.7s
	float Drag = 700.f;           // soltar o acelerador desacelera em ~6.5s, nao 15s
	float MaxSpeed = 6000.f;
	float BoostMaxSpeed = 9000.f;    // teto so alcancavel em boost (1.5x a maxima normal)
	float BoostAcceleration = 5000.f;
	float MinSpeed = 0.f;         // sem piso: o aviao so anda se voce acelerar
	float PitchRateDegPerSec = 110.f;
	float YawRateDegPerSec = 110.f;  // igual ao pitch: mirar em 3D fica simetrico
	float RollRateDegPerSec = 200.f;
	float MaxPitchDeg = 85.f;
};

struct FFlightPhysicsState
{
	float Speed = 0.f;
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

private:
	FFlightPhysicsParams Params;
};
