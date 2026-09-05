#include "FlightPhysics.h"

namespace
{
	float ClampValue(float Value, float Min, float Max)
	{
		if (Value < Min) return Min;
		if (Value > Max) return Max;
		return Value;
	}

	float WrapDegrees(float Degrees)
	{
		float Wrapped = Degrees;
		while (Wrapped >= 360.f) Wrapped -= 360.f;
		while (Wrapped < 0.f) Wrapped += 360.f;
		return Wrapped;
	}
}

void FFlightPhysics::Update(FFlightPhysicsState& State, float ThrottleInput, float PitchInput, float YawInput, float RollInput, float DeltaSeconds) const
{
	ThrottleInput = ClampValue(ThrottleInput, -1.f, 1.f);
	PitchInput = ClampValue(PitchInput, -1.f, 1.f);
	YawInput = ClampValue(YawInput, -1.f, 1.f);
	RollInput = ClampValue(RollInput, -1.f, 1.f);

	if (ThrottleInput > 0.f)
	{
		State.Speed += Params.Acceleration * ThrottleInput * DeltaSeconds;
	}
	else if (ThrottleInput < 0.f)
	{
		State.Speed += Params.Deceleration * ThrottleInput * DeltaSeconds;
	}
	else if (State.Speed > 0.f)
	{
		State.Speed -= Params.Drag * DeltaSeconds;
	}

	State.Speed = ClampValue(State.Speed, Params.MinSpeed, Params.MaxSpeed);

	State.PitchDeg = ClampValue(State.PitchDeg + PitchInput * Params.PitchRateDegPerSec * DeltaSeconds, -Params.MaxPitchDeg, Params.MaxPitchDeg);
	State.YawDeg = WrapDegrees(State.YawDeg + YawInput * Params.YawRateDegPerSec * DeltaSeconds);
	State.RollDeg = WrapDegrees(State.RollDeg + RollInput * Params.RollRateDegPerSec * DeltaSeconds);
}
