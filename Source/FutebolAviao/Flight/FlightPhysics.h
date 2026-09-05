#pragma once

struct FFlightPhysicsParams
{
	float Acceleration = 2600.f;  // 0 -> maxima em ~2.3s
	float Deceleration = 2200.f;  // S freia de verdade: maxima -> piso em ~2s
	float Drag = 700.f;           // soltar o acelerador desacelera em ~6.5s, nao 15s
	float MaxSpeed = 6000.f;
	float MinSpeed = 1400.f;   // piso de cruzeiro: o aviao nunca para no ar
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
	void Update(FFlightPhysicsState& State, float ThrottleInput, float PitchInput, float YawInput, float RollInput, float DeltaSeconds) const;

private:
	FFlightPhysicsParams Params;
};
