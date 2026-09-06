#include "FlightPhysics.h"
#include "PureMath.h"
#include <cmath>

using namespace PureMath;

namespace
{
	constexpr float Pi = 3.14159265358979323846f;

	// Reproduz a formula de FRotator::Vector() da Unreal: so depende de pitch e
	// yaw. Roll nao entra na conta, igual a engine - por isso hoje o roll e
	// puramente decorativo (ver docs/superpowers/specs/2026-09-05-modelo-de-voo.md).
	FFlightVector ForwardFromAngles(float PitchDeg, float YawDeg)
	{
		const float PitchRad = PitchDeg * Pi / 180.f;
		const float YawRad = YawDeg * Pi / 180.f;
		const float CosPitch = std::cos(PitchRad);

		FFlightVector Forward;
		Forward.X = CosPitch * std::cos(YawRad);
		Forward.Y = CosPitch * std::sin(YawRad);
		Forward.Z = std::sin(PitchRad);
		return Forward;
	}

	float Length(const FFlightVector& V)
	{
		return std::sqrt(V.X * V.X + V.Y * V.Y + V.Z * V.Z);
	}
}

void FFlightPhysics::Update(FFlightPhysicsState& State, float ThrottleInput, float PitchInput, float YawInput, float RollInput, bool bBoostActive, float DeltaSeconds) const
{
	ThrottleInput = ClampValue(ThrottleInput, -1.f, 1.f);
	PitchInput = ClampValue(PitchInput, -1.f, 1.f);
	YawInput = ClampValue(YawInput, -1.f, 1.f);
	RollInput = ClampValue(RollInput, -1.f, 1.f);

	// Velocidade e vetor no estado, mas a aceleracao/drag/boost agem sobre o
	// escalar exatamente como antes - so a magnitude muda aqui.
	float Speed = Length(State.Velocity);

	if (bBoostActive)
	{
		Speed += Params.BoostAcceleration * DeltaSeconds;
	}
	else if (ThrottleInput > 0.f)
	{
		Speed += Params.Acceleration * ThrottleInput * DeltaSeconds;
	}
	else if (ThrottleInput < 0.f)
	{
		Speed += Params.Deceleration * ThrottleInput * DeltaSeconds;
	}
	else if (Speed > 0.f)
	{
		Speed -= Params.Drag * DeltaSeconds;
	}

	// Fora do boost o teto e a maxima normal; o excesso ganho em boost e cortado.
	const float SpeedCeiling = bBoostActive ? Params.BoostMaxSpeed : Params.MaxSpeed;
	Speed = ClampValue(Speed, Params.MinSpeed, SpeedCeiling);

	State.PitchDeg = ClampValue(State.PitchDeg + PitchInput * Params.PitchRateDegPerSec * DeltaSeconds, -Params.MaxPitchDeg, Params.MaxPitchDeg);
	State.YawDeg = WrapDegrees(State.YawDeg + YawInput * Params.YawRateDegPerSec * DeltaSeconds);
	State.RollDeg = WrapDegrees(State.RollDeg + RollInput * Params.RollRateDegPerSec * DeltaSeconds);

	// Reaponta o vetor pelos angulos recem-atualizados - o pawn hoje faz esse
	// mesmo calculo logo depois do Update retornar, entao fazer aqui preserva
	// a ordem exata do modelo atual.
	const FFlightVector Forward = ForwardFromAngles(State.PitchDeg, State.YawDeg);
	State.Velocity.X = Forward.X * Speed;
	State.Velocity.Y = Forward.Y * Speed;
	State.Velocity.Z = Forward.Z * Speed;
}

float FFlightPhysics::GetSpeed(const FFlightPhysicsState& State)
{
	return Length(State.Velocity);
}
