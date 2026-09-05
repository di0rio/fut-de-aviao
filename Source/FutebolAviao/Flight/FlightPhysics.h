#pragma once

struct FFlightPhysicsParams
{
	float Acceleration = 2000.f;
	float Deceleration = 1500.f;
	float Drag = 300.f;
	float MaxSpeed = 6000.f;
	float MinSpeed = 0.f;
	float PitchRateDegPerSec = 60.f;
	float YawRateDegPerSec = 90.f;
	float RollRateDegPerSec = 120.f;
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
