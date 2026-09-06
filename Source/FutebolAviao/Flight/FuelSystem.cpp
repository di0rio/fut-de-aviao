#include "FuelSystem.h"
#include "PureMath.h"

using namespace PureMath;

void FFuelSystem::Update(FFuelState& State, bool bBoostActive, float DeltaSeconds) const
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

	if (bBoostActive)
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

bool FFuelSystem::ResolveBoost(const FFuelState& State, bool bBoostRequested) const
{
	return bBoostRequested && CanBoost(State);
}
