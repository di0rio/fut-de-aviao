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
	// Destruido: nao consome, nao regenera, so espera o respawn.
	if (State.bIsDestroyed)
	{
		State.RespawnTimer -= DeltaSeconds;
		if (State.RespawnTimer <= 0.f)
		{
			State.bIsDestroyed = false;
			State.RespawnTimer = 0.f;
			State.Fuel = Params.TankCapacity * Params.RespawnFuelFraction;
			State.TimeSinceBoostSec = 0.f;
		}
		return;
	}

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

	if (State.Fuel <= 0.f && !State.bIsDestroyed)
	{
		State.bIsDestroyed = true;
		State.RespawnTimer = Params.RespawnSeconds;
	}
}

bool FFuelSystem::CanBoost(const FFuelState& State) const
{
	return !State.bIsDestroyed && State.Fuel > 0.f;
}
