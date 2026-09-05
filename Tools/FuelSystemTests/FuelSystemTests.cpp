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

int main()
{
	Test_BoostDrainsFuel();
	printf("All tests passed\n");
	return 0;
}
