#include "FuelSystem.h"

namespace
{
	float ClampValue(float Value, float Min, float Max)
	{
		if (Value < Min) return Min;
		if (Value > Max) return Max;
		return Value;
	}
}

void FFuelSystem::Update(FFuelState& State, bool bBoostInput, float DeltaSeconds) const
{
	if (bBoostInput)
	{
		State.Fuel -= Params.BoostDrainPerSec * DeltaSeconds;
		State.TimeSinceBoostSec = 0.f;
	}
	else
	{
		State.TimeSinceBoostSec += DeltaSeconds;
		if (State.TimeSinceBoostSec >= Params.RegenDelayAfterBoostSec)
		{
			State.Fuel += Params.PassiveRegenPerSec * DeltaSeconds;
		}
	}

	State.Fuel = ClampValue(State.Fuel, 0.f, Params.TankCapacity);
}

bool FFuelSystem::CanBoost(const FFuelState& State) const
{
	return !State.bIsDestroyed && State.Fuel > 0.f;
}
