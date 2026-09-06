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
	Params.Arena.ArenaHalfX = 100000.f;
	Params.Arena.ArenaHalfY = 100000.f;
	Params.Arena.ArenaCeilingZ = 100000.f;
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

static void Test_BallFlyingIntoTheGoalMouthPassesThrough()
{
	FBallPhysicsParams Params;
	Params.Gravity = 0.f;
	Params.Drag = 0.f;
	Params.Radius = 150.f;
	// A bola precisa saber onde fica a boca do gol (FArenaGeometry, embutida em
	// Params.Arena) pra nao quicar bem onde deveria ser a entrada. Este teste
	// mira bem no meio da boca (Y perto de zero, Z bem abaixo do travessao)
	// pra provar isso.
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.X = 9900.f;   // perto da parede de fundo (ArenaHalfX = 10000)
	State.Position.Y = 0.f;      // centro da boca do gol
	State.Position.Z = 1000.f;   // bem abaixo do travessao
	State.Velocity.X = 5000.f;   // rapido o bastante pra passar da linha em 1s

	Ball.Update(State, 1.f);

	assert(State.Position.X > Params.Arena.ArenaHalfX);   // atravessou, nao quicou de volta
	printf("Test_BallFlyingIntoTheGoalMouthPassesThrough passed\n");
}

static void Test_IsOverlappingDetectsContactAndSeparation()
{
	FBallPhysicsParams Params;
	Params.Radius = 150.f;
	FBallPhysics Ball(Params);
	FBallState State;   // bola na origem

	const float PlaneRadius = 300.f;
	const float RadiiSum = Params.Radius + PlaneRadius;   // 450

	PureMath::FPureVector Plane;

	// Centros bem mais pertos que a soma dos raios: claramente sobrepondo.
	Plane.X = 100.f;
	assert(Ball.IsOverlapping(State, Plane, PlaneRadius) == true);

	// Centros bem mais longe que a soma dos raios: claramente separadas.
	Plane.X = 1000.f;
	assert(Ball.IsOverlapping(State, Plane, PlaneRadius) == false);

	// Um pouco dentro da fronteira (distancia < soma dos raios): ainda sobrepondo.
	Plane.X = RadiiSum - 1.f;
	assert(Ball.IsOverlapping(State, Plane, PlaneRadius) == true);

	// Um pouco fora da fronteira (distancia > soma dos raios): ja separadas.
	Plane.X = RadiiSum + 1.f;
	assert(Ball.IsOverlapping(State, Plane, PlaneRadius) == false);

	printf("Test_IsOverlappingDetectsContactAndSeparation passed\n");
}

static void Test_BallHittingTheBackWallOutsideTheMouthStillBounces()
{
	FBallPhysicsParams Params;
	Params.Gravity = 0.f;
	Params.Drag = 0.f;
	Params.Radius = 150.f;
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.X = 9900.f;   // mesma aproximacao do teste anterior
	State.Position.Y = 4000.f;   // bem longe da boca do gol (GoalHalfWidthY = 1500)
	State.Position.Z = 1000.f;
	State.Velocity.X = 5000.f;

	Ball.Update(State, 1.f);

	assert(State.Velocity.X < 0.f);   // quicou: velocidade inverteu
	printf("Test_BallHittingTheBackWallOutsideTheMouthStillBounces passed\n");
}

int main()
{
	Test_GravityPullsTheBallDown();
	Test_BallBouncesOffTheFloorLosingEnergy();
	Test_DragSlowsTheBallDown();
	Test_PlaneHitPushesTheBallAway();
	Test_StationaryPlaneStillNudgesTheBall();
	Test_BallFlyingIntoTheGoalMouthPassesThrough();
	Test_IsOverlappingDetectsContactAndSeparation();
	Test_BallHittingTheBackWallOutsideTheMouthStillBounces();
	printf("All tests passed\n");
	return 0;
}
