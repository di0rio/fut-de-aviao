#include "../../Source/FutebolAviao/Flight/FuelSystem.h"
#include <cassert>
#include <cstdio>
#include <cmath>

static bool NearlyEqual(float A, float B, float Tolerance = 0.01f)
{
	return std::fabs(A - B) <= Tolerance;
}

static void Test_BoostDrainsFuel()
{
	FFuelParams Params;
	Params.TankCapacity = 100.f;
	Params.BoostDrainPerSec = 25.f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 100.f;

	Fuel.Update(State, /*bBoostInput*/ true, /*DeltaSeconds*/ 1.f);

	assert(NearlyEqual(State.Fuel, 75.f));
	printf("Test_BoostDrainsFuel passed\n");
}

static void Test_NotBoostingRegeneratesFuel()
{
	FFuelParams Params;
	Params.PassiveRegenPerSec = 8.f;
	Params.RegenDelayAfterBoostSec = 1.5f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 50.f;

	Fuel.Update(State, false, 1.f);

	assert(NearlyEqual(State.Fuel, 58.f));
	printf("Test_NotBoostingRegeneratesFuel passed\n");
}

static void Test_RegenIsSuppressedRightAfterBoosting()
{
	FFuelParams Params;
	Params.TankCapacity = 100.f;
	Params.BoostDrainPerSec = 25.f;
	Params.PassiveRegenPerSec = 8.f;
	Params.RegenDelayAfterBoostSec = 1.5f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 50.f;

	Fuel.Update(State, true, 1.f);    // gasta 25 -> 25 de combustivel, zera o relogio
	Fuel.Update(State, false, 1.f);   // 1s < 1.5s de delay: nao pode regenerar ainda

	assert(NearlyEqual(State.Fuel, 25.f));
	printf("Test_RegenIsSuppressedRightAfterBoosting passed\n");
}

static void Test_FuelNeverExceedsTankCapacity()
{
	FFuelParams Params;
	Params.TankCapacity = 100.f;
	Params.PassiveRegenPerSec = 500.f;
	Params.RegenDelayAfterBoostSec = 0.f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 90.f;

	Fuel.Update(State, false, 1.f);

	assert(NearlyEqual(State.Fuel, 100.f));
	printf("Test_FuelNeverExceedsTankCapacity passed\n");
}

static void Test_EmptyTankDestroysThePlane()
{
	FFuelParams Params;
	Params.BoostDrainPerSec = 25.f;
	Params.RespawnSeconds = 3.f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 10.f;

	Fuel.Update(State, true, 1.f);   // pede 25 de dreno com so 10 no tanque

	assert(NearlyEqual(State.Fuel, 0.f));
	assert(State.bIsDestroyed);
	assert(NearlyEqual(State.RespawnTimer, 3.f));
	printf("Test_EmptyTankDestroysThePlane passed\n");
}

static void Test_PlaneRespawnsWithHalfTankAfterTimer()
{
	FFuelParams Params;
	Params.TankCapacity = 100.f;
	Params.RespawnSeconds = 3.f;
	Params.RespawnFuelFraction = 0.5f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 0.f;
	State.bIsDestroyed = true;
	State.RespawnTimer = 3.f;

	Fuel.Update(State, false, 1.f);
	assert(State.bIsDestroyed);              // 2s restantes: ainda fora da jogada

	Fuel.Update(State, false, 1.f);
	Fuel.Update(State, false, 1.f);

	assert(!State.bIsDestroyed);
	assert(NearlyEqual(State.Fuel, 50.f));
	printf("Test_PlaneRespawnsWithHalfTankAfterTimer passed\n");
}

static void Test_CanBoostIsFalseWhileDestroyedOrEmpty()
{
	FFuelSystem Fuel((FFuelParams()));

	FFuelState Alive;
	Alive.Fuel = 10.f;
	Alive.bIsDestroyed = false;
	assert(Fuel.CanBoost(Alive));

	FFuelState Empty;
	Empty.Fuel = 0.f;
	Empty.bIsDestroyed = false;
	assert(!Fuel.CanBoost(Empty));

	FFuelState Destroyed;
	Destroyed.Fuel = 50.f;
	Destroyed.bIsDestroyed = true;
	assert(!Fuel.CanBoost(Destroyed));

	printf("Test_CanBoostIsFalseWhileDestroyedOrEmpty passed\n");
}

int main()
{
	Test_BoostDrainsFuel();
	Test_NotBoostingRegeneratesFuel();
	Test_RegenIsSuppressedRightAfterBoosting();
	Test_FuelNeverExceedsTankCapacity();
	Test_EmptyTankDestroysThePlane();
	Test_PlaneRespawnsWithHalfTankAfterTimer();
	Test_CanBoostIsFalseWhileDestroyedOrEmpty();
	printf("All tests passed\n");
	return 0;
}
