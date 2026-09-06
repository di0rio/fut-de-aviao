#include "../../Source/FutebolAviao/Flight/BallPhysics.h"
#include <cassert>
#include <cstdio>
#include <cmath>

static bool NearlyEqual(float A, float B, float Tolerance = 0.01f)
{
	return std::fabs(A - B) <= Tolerance;
}

static void Test_GravityPullsTheBallDown()
{
	FBallPhysicsParams Params;
	Params.Gravity = 980.f;
	Params.Drag = 0.f;
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.Z = 3000.f;

	Ball.Update(State, 1.f);

	assert(NearlyEqual(State.Velocity.Z, -980.f));
	printf("Test_GravityPullsTheBallDown passed\n");
}

int main()
{
	Test_GravityPullsTheBallDown();
	printf("All tests passed\n");
	return 0;
}
