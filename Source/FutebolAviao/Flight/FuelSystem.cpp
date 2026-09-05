#include "FuelSystem.h"

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
}

bool FFuelSystem::CanBoost(const FFuelState& State) const
{
	return !State.bIsDestroyed && State.Fuel > 0.f;
}
