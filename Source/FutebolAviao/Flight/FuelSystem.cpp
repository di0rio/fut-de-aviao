#include "FuelSystem.h"

void FFuelSystem::Update(FFuelState& State, bool bBoostInput, float DeltaSeconds) const
{
	if (bBoostInput)
	{
		State.Fuel -= Params.BoostDrainPerSec * DeltaSeconds;
	}
}

bool FFuelSystem::CanBoost(const FFuelState& State) const
{
	return !State.bIsDestroyed && State.Fuel > 0.f;
}
