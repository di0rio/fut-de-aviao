#include "../../Source/FutebolAviao/Flight/FlightPhysics.h"
#include <cassert>
#include <cstdio>
#include <cmath>

static bool NearlyEqual(float A, float B, float Tolerance = 0.01f)
{
	return std::fabs(A - B) <= Tolerance;
}

static void Test_ThrottleAcceleratesSpeed()
{
	FFlightPhysicsParams Params;
	Params.MinSpeed = 0.f; // isola o teste do piso de cruzeiro
	Params.Acceleration = 1000.f;
	Params.MaxSpeed = 10000.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;

	Physics.Update(State, /*Throttle*/ 1.f, 0.f, 0.f, 0.f, /*DeltaSeconds*/ 1.f);

	assert(NearlyEqual(State.Speed, 1000.f));
	printf("Test_ThrottleAcceleratesSpeed passed\n");
}

static void Test_SpeedClampsToMaxSpeed()
{
	FFlightPhysicsParams Params;
	Params.MinSpeed = 0.f; // isola o teste do piso de cruzeiro
	Params.Acceleration = 100000.f;
	Params.MaxSpeed = 500.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;

	Physics.Update(State, 1.f, 0.f, 0.f, 0.f, 1.f);

	assert(NearlyEqual(State.Speed, 500.f));
	printf("Test_SpeedClampsToMaxSpeed passed\n");
}

static void Test_NoThrottleAppliesDrag()
{
	FFlightPhysicsParams Params;
	Params.MinSpeed = 0.f; // isola o teste do piso de cruzeiro
	Params.Drag = 200.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;
	State.Speed = 1000.f;

	Physics.Update(State, 0.f, 0.f, 0.f, 0.f, 1.f);

	assert(NearlyEqual(State.Speed, 800.f));
	printf("Test_NoThrottleAppliesDrag passed\n");
}

static void Test_SpeedNeverGoesNegativeFromDrag()
{
	FFlightPhysicsParams Params;
	Params.MinSpeed = 0.f; // isola o teste do piso de cruzeiro
	Params.Drag = 200.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;
	State.Speed = 50.f;

	Physics.Update(State, 0.f, 0.f, 0.f, 0.f, 1.f);

	assert(NearlyEqual(State.Speed, 0.f));
	printf("Test_SpeedNeverGoesNegativeFromDrag passed\n");
}

static void Test_PitchInputRotatesNoseAndClamps()
{
	FFlightPhysicsParams Params;
	Params.PitchRateDegPerSec = 90.f;
	Params.MaxPitchDeg = 85.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;

	Physics.Update(State, 0.f, 1.f, 0.f, 0.f, 1.f); // 90 deg pedido, clampa em 85
	assert(NearlyEqual(State.PitchDeg, 85.f));
	printf("Test_PitchInputRotatesNoseAndClamps passed\n");
}

static void Test_YawInputWrapsAround360()
{
	FFlightPhysicsParams Params;
	Params.YawRateDegPerSec = 350.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;
	State.YawDeg = 20.f;

	Physics.Update(State, 0.f, 0.f, 1.f, 0.f, 1.f); // 20 + 350 = 370 -> wrap pra 10
	assert(NearlyEqual(State.YawDeg, 10.f));
	printf("Test_YawInputWrapsAround360 passed\n");
}

int main()
{
	Test_ThrottleAcceleratesSpeed();
	Test_SpeedClampsToMaxSpeed();
	Test_NoThrottleAppliesDrag();
	Test_SpeedNeverGoesNegativeFromDrag();
	Test_PitchInputRotatesNoseAndClamps();
	Test_YawInputWrapsAround360();
	printf("All tests passed\n");
	return 0;
}
