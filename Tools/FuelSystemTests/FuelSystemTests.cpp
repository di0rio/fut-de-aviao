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

int main()
{
	Test_BoostDrainsFuel();
	Test_NotBoostingRegeneratesFuel();
	Test_RegenIsSuppressedRightAfterBoosting();
	printf("All tests passed\n");
	return 0;
}
