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
	FPureVector ForwardFromAngles(float PitchDeg, float YawDeg)
	{
		const float PitchRad = PitchDeg * Pi / 180.f;
		const float YawRad = YawDeg * Pi / 180.f;
		const float CosPitch = std::cos(PitchRad);

		FPureVector Forward;
		Forward.X = CosPitch * std::cos(YawRad);
		Forward.Y = CosPitch * std::sin(YawRad);
		Forward.Z = std::sin(PitchRad);
		return Forward;
	}

	// Abaixo disso o vetor nao carrega direcao confiavel (velocidade em
	// repouso, ou um blend que quase cancelou por CurrentDir e NoseDir serem
	// quase opostos) - normalizar dividiria por quase-zero. Fallback e sempre
	// o nariz.
	constexpr float DirectionEpsilon = 1e-4f;

	// Mantido local em vez de PureMath::Normalized: aqui o fallback e o
	// parametro Fallback (o nariz), nao o vetor zero, e a divisao e feita
	// componente a componente (V.X / Len) em vez de multiplicar por 1/Len.
	// Trocar pela versao de PureMath mudaria o resultado por arredondamento
	// de ponto flutuante, o que esta task nao pode fazer.
	FPureVector Normalize(const FPureVector& V, const FPureVector& Fallback)
	{
		const float Len = Length(V);
		if (Len < DirectionEpsilon)
		{
			return Fallback;
		}

		FPureVector Result;
		Result.X = V.X / Len;
		Result.Y = V.Y / Len;
		Result.Z = V.Z / Len;
		return Result;
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

	// Passo 2 (inercia): a velocidade nao reaponta pro nariz instantaneamente
	// mais - ela persegue por alinhamento exponencial, o que da peso ao aviao
	// e cria deriva na curva. So a DIRECAO muda aqui; Speed acima e a mesma
	// logica de sempre.
	const PureMath::FPureVector NoseDir = ForwardFromAngles(State.PitchDeg, State.YawDeg);

	// Direcao atual ANTES do blend, capturada da velocidade que ainda esta no
	// estado (do frame anterior). MinSpeed=0 e normal (decidido em playtest),
	// entao velocidade em repouso nao e bug - so nao tem direcao confiavel pra
	// normalizar, e o nariz vira a direcao.
	const float PreviousSpeed = Length(State.Velocity);
	const PureMath::FPureVector CurrentDir = (PreviousSpeed > DirectionEpsilon) ? Normalize(State.Velocity, NoseDir) : NoseDir;

	// Alinhamento exponencial: Alpha cresce com DeltaSeconds e satura em 1.0
	// pra taxas de alinhamento altas (exp(-x) vira ~0 em float bem antes do
	// limite de precisao), entao uma taxa bem alta reproduz o modelo antigo
	// (velocidade solda no nariz todo frame) de novo.
	const float Alpha = ClampValue(1.f - std::exp(-Params.VelocityAlignPerSec * DeltaSeconds), 0.f, 1.f);

	PureMath::FPureVector Blended;
	Blended.X = CurrentDir.X + (NoseDir.X - CurrentDir.X) * Alpha;
	Blended.Y = CurrentDir.Y + (NoseDir.Y - CurrentDir.Y) * Alpha;
	Blended.Z = CurrentDir.Z + (NoseDir.Z - CurrentDir.Z) * Alpha;

	// Guarda: se CurrentDir e NoseDir forem quase opostos o blend pode
	// cancelar quase tudo e sobrar um vetor curto demais pra normalizar com
	// precisao - cai pro nariz nesse caso.
	const PureMath::FPureVector BlendedDir = Normalize(Blended, NoseDir);

	State.Velocity.X = BlendedDir.X * Speed;
	State.Velocity.Y = BlendedDir.Y * Speed;
	State.Velocity.Z = BlendedDir.Z * Speed;
}

float FFlightPhysics::GetSpeed(const FFlightPhysicsState& State)
{
	return Length(State.Velocity);
}
