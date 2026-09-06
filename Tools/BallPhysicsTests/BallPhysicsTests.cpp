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

static void Test_BallBouncesOffTheFloorLosingEnergy()
{
	FBallPhysicsParams Params;
	Params.Gravity = 0.f;
	Params.Drag = 0.f;
	Params.Restitution = 0.75f;
	Params.Radius = 150.f;
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.Z = 200.f;
	State.Velocity.Z = -100.f;

	Ball.Update(State, 1.f);   // desceria pra 100, abaixo do raio: quica

	assert(NearlyEqual(State.Position.Z, 150.f));   // pousada em cima do chao
	assert(NearlyEqual(State.Velocity.Z, 75.f));    // 100 * 0.75, agora subindo
	printf("Test_BallBouncesOffTheFloorLosingEnergy passed\n");
}

static void Test_DragSlowsTheBallDown()
{
	FBallPhysicsParams Params;
	Params.Gravity = 0.f;
	Params.Drag = 0.5f;
	Params.ArenaHalfX = 100000.f;
	Params.ArenaHalfY = 100000.f;
	Params.ArenaCeilingZ = 100000.f;
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.Z = 5000.f;
	State.Velocity.X = 1000.f;

	Ball.Update(State, 1.f);

	assert(State.Velocity.X < 1000.f);
	assert(State.Velocity.X > 0.f);   // desacelera, nao inverte nem zera
	printf("Test_DragSlowsTheBallDown passed\n");
}

static void Test_PlaneHitPushesTheBallAway()
{
	FBallPhysicsParams Params;
	Params.Radius = 150.f;
	Params.HitTransfer = 1.6f;
	Params.MinKick = 500.f;
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.X = 400.f;   // bola a frente do aviao, no eixo X

	PureMath::FPureVector PlanePosition;   // aviao na origem
	PureMath::FPureVector PlaneVelocity;
	PlaneVelocity.X = 3000.f;              // voando em cima dela

	Ball.ApplyHit(State, PlanePosition, PlaneVelocity, /*PlaneRadius*/ 300.f);

	assert(State.Velocity.X > 3000.f);                  // levou impulso pra frente
	assert(NearlyEqual(State.Velocity.Y, 0.f));
	assert(NearlyEqual(State.Position.X, 450.f));       // empurrada pra fora da sobreposicao
	printf("Test_PlaneHitPushesTheBallAway passed\n");
}

static void Test_StationaryPlaneStillNudgesTheBall()
{
	FBallPhysicsParams Params;
	Params.Radius = 150.f;
	Params.MinKick = 500.f;
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.X = 400.f;

	PureMath::FPureVector PlanePosition;
	PureMath::FPureVector PlaneVelocity;   // aviao parado

	Ball.ApplyHit(State, PlanePosition, PlaneVelocity, 300.f);

	assert(NearlyEqual(State.Velocity.X, 500.f));   // so o MinKick
	printf("Test_StationaryPlaneStillNudgesTheBall passed\n");
}

int main()
{
	Test_GravityPullsTheBallDown();
	Test_BallBouncesOffTheFloorLosingEnergy();
	Test_DragSlowsTheBallDown();
	Test_PlaneHitPushesTheBallAway();
	Test_StationaryPlaneStillNudgesTheBall();
	printf("All tests passed\n");
	return 0;
}
