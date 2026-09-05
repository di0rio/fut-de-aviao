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

	Physics.Update(State, /*Throttle*/ 1.f, 0.f, 0.f, 0.f, /*bBoostActive*/ false, /*DeltaSeconds*/ 1.f);

	assert(NearlyEqual(FFlightPhysics::GetSpeed(State), 1000.f));
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

	Physics.Update(State, 1.f, 0.f, 0.f, 0.f, false, 1.f);

	assert(NearlyEqual(FFlightPhysics::GetSpeed(State), 500.f));
	printf("Test_SpeedClampsToMaxSpeed passed\n");
}

static void Test_NoThrottleAppliesDrag()
{
	FFlightPhysicsParams Params;
	Params.MinSpeed = 0.f; // isola o teste do piso de cruzeiro
	Params.Drag = 200.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;
	State.Velocity = { 1000.f, 0.f, 0.f }; // angulos default (0,0): equivalente ao Speed=1000 de antes

	Physics.Update(State, 0.f, 0.f, 0.f, 0.f, false, 1.f);

	assert(NearlyEqual(FFlightPhysics::GetSpeed(State), 800.f));
	printf("Test_NoThrottleAppliesDrag passed\n");
}

static void Test_SpeedNeverGoesNegativeFromDrag()
{
	FFlightPhysicsParams Params;
	Params.MinSpeed = 0.f; // isola o teste do piso de cruzeiro
	Params.Drag = 200.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;
	State.Velocity = { 50.f, 0.f, 0.f }; // angulos default (0,0): equivalente ao Speed=50 de antes

	Physics.Update(State, 0.f, 0.f, 0.f, 0.f, false, 1.f);

	assert(NearlyEqual(FFlightPhysics::GetSpeed(State), 0.f));
	printf("Test_SpeedNeverGoesNegativeFromDrag passed\n");
}

static void Test_PitchInputRotatesNoseAndClamps()
{
	FFlightPhysicsParams Params;
	Params.PitchRateDegPerSec = 90.f;
	Params.MaxPitchDeg = 85.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;

	Physics.Update(State, 0.f, 1.f, 0.f, 0.f, false, 1.f); // 90 deg pedido, clampa em 85
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

	Physics.Update(State, 0.f, 0.f, 1.f, 0.f, false, 1.f); // 20 + 350 = 370 -> wrap pra 10
	assert(NearlyEqual(State.YawDeg, 10.f));
	printf("Test_YawInputWrapsAround360 passed\n");
}

static void Test_BoostAcceleratesPastNormalMaxSpeed()
{
	FFlightPhysicsParams Params;
	Params.MinSpeed = 0.f;
	Params.MaxSpeed = 6000.f;
	Params.BoostMaxSpeed = 9000.f;
	Params.BoostAcceleration = 5000.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;
	State.Velocity = { 6000.f, 0.f, 0.f }; // angulos default (0,0): equivalente ao Speed=6000 de antes

	Physics.Update(State, 1.f, 0.f, 0.f, 0.f, /*bBoostActive*/ true, 1.f);

	assert(NearlyEqual(FFlightPhysics::GetSpeed(State), 9000.f)); // 6000 + 5000 = 11000, clampado no teto do boost
	printf("Test_BoostAcceleratesPastNormalMaxSpeed passed\n");
}

static void Test_BoostOverridesBrakeInput()
{
	// Decisao de design confirmada em playtest: boost e compromisso, o freio nao
	// corta ele. Se algum dia mudar, este teste e o lugar de registrar a mudanca.
	FFlightPhysicsParams Params;
	Params.MinSpeed = 0.f;
	Params.MaxSpeed = 6000.f;
	Params.BoostMaxSpeed = 9000.f;
	Params.BoostAcceleration = 5000.f;
	Params.Deceleration = 2200.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;
	State.Velocity = { 6000.f, 0.f, 0.f }; // angulos default (0,0): equivalente ao Speed=6000 de antes

	Physics.Update(State, /*Throttle*/ -1.f, 0.f, 0.f, 0.f, /*bBoostActive*/ true, 1.f);

	assert(NearlyEqual(FFlightPhysics::GetSpeed(State), 9000.f));
	printf("Test_BoostOverridesBrakeInput passed\n");
}

static void Test_VelocityVectorMatchesLegacyNoseDirectionModel()
{
	// Prova o Passo 1 de docs/superpowers/specs/2026-09-05-modelo-de-voo.md:
	// trocar Speed escalar por um vetor no estado nao pode mudar a trajetoria.
	//
	// Este teste roda a MESMA sequencia de inputs mistos (throttle, pitch, yaw,
	// roll, incluindo um passo com boost) pelo FFlightPhysics::Update de
	// verdade, e em paralelo por uma reimplementacao longhand do modelo antigo
	// (velocidade escalar + Forward calculado FORA do Update, como o pawn
	// fazia antes desta refatoracao). A reimplementacao abaixo nao chama
	// nenhuma funcao de FlightPhysics.cpp nem FFlightPhysics::GetSpeed pra
	// produzir o valor esperado - se chamasse, o teste so provaria que o
	// codigo concorda consigo mesmo.
	constexpr float LocalPi = 3.14159265358979323846f;

	struct FVec3 { float X = 0.f; float Y = 0.f; float Z = 0.f; };

	auto ClampValueLocal = [](float Value, float Min, float Max) -> float
	{
		if (Value < Min) return Min;
		if (Value > Max) return Max;
		return Value;
	};

	auto WrapDegreesLocal = [](float Degrees) -> float
	{
		float Wrapped = Degrees;
		while (Wrapped >= 360.f) Wrapped -= 360.f;
		while (Wrapped < 0.f) Wrapped += 360.f;
		return Wrapped;
	};

	auto ForwardFromAnglesLocal = [LocalPi](float PitchDeg, float YawDeg) -> FVec3
	{
		const float PitchRad = PitchDeg * LocalPi / 180.f;
		const float YawRad = YawDeg * LocalPi / 180.f;
		const float CosPitch = std::cos(PitchRad);
		FVec3 Forward;
		Forward.X = CosPitch * std::cos(YawRad);
		Forward.Y = CosPitch * std::sin(YawRad);
		Forward.Z = std::sin(PitchRad);
		return Forward;
	};

	FFlightPhysicsParams Params; // parametros default, compartilhados pelos dois modelos
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State; // modelo novo: FFlightPhysics::Update mexe nisto

	// Modelo legado, reimplementado a mao aqui dentro: so escalar, sem vetor.
	float LegacySpeed = 0.f;
	float LegacyPitchDeg = 0.f;
	float LegacyYawDeg = 0.f;
	float LegacyRollDeg = 0.f;
	FVec3 LegacyPos;
	FVec3 NewPos;

	struct FStep { float Throttle; float Pitch; float Yaw; float Roll; bool bBoost; float Dt; };
	const FStep Steps[] =
	{
		{  1.f,   0.f,   0.f,  0.f, false, 0.1f },  // acelera reto
		{  1.f,   0.5f,  0.f,  0.f, false, 0.1f },  // acelera + sobe o nariz
		{  0.f,   0.f,   1.f,  0.f, false, 0.1f },  // solta o acelerador, vira (drag entra)
		{ -1.f,  -0.3f,  0.5f, 1.f, false, 0.1f },  // freia, mistura pitch/yaw/roll
		{  1.f,   0.2f, -0.4f, -1.f, true,  0.1f },  // passo com boost
		{  0.f,   0.f,   0.f,  0.f, false, 0.1f },  // solta tudo, so drag
	};

	for (const FStep& S : Steps)
	{
		// --- modelo novo: o de verdade, via FFlightPhysics::Update ---
		Physics.Update(State, S.Throttle, S.Pitch, S.Yaw, S.Roll, S.bBoost, S.Dt);
		NewPos.X += State.Velocity.X * S.Dt;
		NewPos.Y += State.Velocity.Y * S.Dt;
		NewPos.Z += State.Velocity.Z * S.Dt;

		// --- modelo legado: escalar + Forward calculado depois do "Update" ---
		const float Throttle = ClampValueLocal(S.Throttle, -1.f, 1.f);
		const float PitchInput = ClampValueLocal(S.Pitch, -1.f, 1.f);
		const float YawInput = ClampValueLocal(S.Yaw, -1.f, 1.f);
		const float RollInput = ClampValueLocal(S.Roll, -1.f, 1.f);

		if (S.bBoost)
		{
			LegacySpeed += Params.BoostAcceleration * S.Dt;
		}
		else if (Throttle > 0.f)
		{
			LegacySpeed += Params.Acceleration * Throttle * S.Dt;
		}
		else if (Throttle < 0.f)
		{
			LegacySpeed += Params.Deceleration * Throttle * S.Dt;
		}
		else if (LegacySpeed > 0.f)
		{
			LegacySpeed -= Params.Drag * S.Dt;
		}

		const float SpeedCeiling = S.bBoost ? Params.BoostMaxSpeed : Params.MaxSpeed;
		LegacySpeed = ClampValueLocal(LegacySpeed, Params.MinSpeed, SpeedCeiling);

		LegacyPitchDeg = ClampValueLocal(LegacyPitchDeg + PitchInput * Params.PitchRateDegPerSec * S.Dt, -Params.MaxPitchDeg, Params.MaxPitchDeg);
		LegacyYawDeg = WrapDegreesLocal(LegacyYawDeg + YawInput * Params.YawRateDegPerSec * S.Dt);
		LegacyRollDeg = WrapDegreesLocal(LegacyRollDeg + RollInput * Params.RollRateDegPerSec * S.Dt);
		(void)LegacyRollDeg; // integrado por fidelidade ao modelo antigo, mas nao entra no Forward (roll e decorativo)

		const FVec3 Forward = ForwardFromAnglesLocal(LegacyPitchDeg, LegacyYawDeg);
		LegacyPos.X += Forward.X * LegacySpeed * S.Dt;
		LegacyPos.Y += Forward.Y * LegacySpeed * S.Dt;
		LegacyPos.Z += Forward.Z * LegacySpeed * S.Dt;
	}

	assert(NearlyEqual(FFlightPhysics::GetSpeed(State), LegacySpeed, 0.5f));
	assert(NearlyEqual(NewPos.X, LegacyPos.X, 0.5f));
	assert(NearlyEqual(NewPos.Y, LegacyPos.Y, 0.5f));
	assert(NearlyEqual(NewPos.Z, LegacyPos.Z, 0.5f));
	printf("Test_VelocityVectorMatchesLegacyNoseDirectionModel passed\n");
}

int main()
{
	Test_ThrottleAcceleratesSpeed();
	Test_SpeedClampsToMaxSpeed();
	Test_NoThrottleAppliesDrag();
	Test_SpeedNeverGoesNegativeFromDrag();
	Test_PitchInputRotatesNoseAndClamps();
	Test_YawInputWrapsAround360();
	Test_BoostAcceleratesPastNormalMaxSpeed();
	Test_BoostOverridesBrakeInput();
	Test_VelocityVectorMatchesLegacyNoseDirectionModel();
	printf("All tests passed\n");
	return 0;
}
