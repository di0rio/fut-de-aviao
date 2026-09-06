#include "BallPhysics.h"

namespace
{
	// Quica um eixo contra um limite. Devolve true se houve quique.
	bool BounceAxis(float& Coordinate, float& Velocity, float Min, float Max, float Restitution)
	{
		if (Coordinate < Min)
		{
			Coordinate = Min;
			Velocity = -Velocity * Restitution;
			return true;
		}
		if (Coordinate > Max)
		{
			Coordinate = Max;
			Velocity = -Velocity * Restitution;
			return true;
		}
		return false;
	}

	// Normal do impacto: da bola pro aviao, com os mesmos fallbacks usados por
	// ApplyHit de sempre. Compartilhada por ApplyImpulse e PushOutOf pra que,
	// chamadas em sequencia sem a posicao mudar entre elas, produzam a mesma
	// normal que o ApplyHit combinado sempre produziu.
	PureMath::FPureVector ComputeHitNormal(const PureMath::FPureVector& BallPosition, const PureMath::FPureVector& PlanePosition, const PureMath::FPureVector& PlaneVelocity)
	{
		PureMath::FPureVector Normal = PureMath::Normalized(PureMath::Subtract(BallPosition, PlanePosition));
		if (PureMath::Length(Normal) < 0.5f)
		{
			// Aviao exatamente em cima da bola: nao ha direcao definida. Chuta pra
			// frente do aviao, que e a unica direcao com significado aqui.
			Normal = PureMath::Normalized(PlaneVelocity);
			if (PureMath::Length(Normal) < 0.5f)
			{
				Normal.Z = 1.f;   // aviao parado tambem: joga pra cima
			}
		}
		return Normal;
	}
}

void FBallPhysics::Update(FBallState& State, float DeltaSeconds) const
{
	State.Velocity.Z -= Params.Gravity * DeltaSeconds;

	// Arrasto exponencial: fracao da velocidade perdida por segundo, estavel em
	// qualquer DeltaSeconds (ao contrario de subtrair uma constante).
	const float DragFactor = PureMath::ClampValue(1.f - Params.Drag * DeltaSeconds, 0.f, 1.f);
	State.Velocity = PureMath::Scale(State.Velocity, DragFactor);

	const float Speed = PureMath::Length(State.Velocity);
	if (Speed > Params.MaxSpeed)
	{
		State.Velocity = PureMath::Scale(PureMath::Normalized(State.Velocity), Params.MaxSpeed);
	}

	State.Position = PureMath::Add(State.Position, PureMath::Scale(State.Velocity, DeltaSeconds));

	const float R = Params.Radius;

	// A boca do gol e um buraco na parede de fundo: dentro dela, o eixo X nao
	// quica -- a bola deve atravessar pra dentro do gol em vez de bater numa
	// parede que ali nao existe.
	const bool bInsideGoalMouth =
		std::fabs(State.Position.Y) < Params.Arena.GoalHalfWidthY &&
		State.Position.Z < Params.Arena.GoalHeightZ;

	if (!bInsideGoalMouth)
	{
		BounceAxis(State.Position.X, State.Velocity.X, -Params.Arena.ArenaHalfX + R, Params.Arena.ArenaHalfX - R, Params.Restitution);
	}
	BounceAxis(State.Position.Y, State.Velocity.Y, -Params.Arena.ArenaHalfY + R, Params.Arena.ArenaHalfY - R, Params.Restitution);
	BounceAxis(State.Position.Z, State.Velocity.Z, R, Params.Arena.ArenaCeilingZ - R, Params.Restitution);
}

bool FBallPhysics::IsOverlapping(const FBallState& State, const PureMath::FPureVector& PlanePosition, float PlaneRadius) const
{
	const float Distance = PureMath::Length(PureMath::Subtract(State.Position, PlanePosition));
	return Distance < (Params.Radius + PlaneRadius);
}

void FBallPhysics::ApplyHit(FBallState& State, const PureMath::FPureVector& PlanePosition, const PureMath::FPureVector& PlaneVelocity, float PlaneRadius) const
{
	ApplyImpulse(State, PlanePosition, PlaneVelocity);
	PushOutOf(State, PlanePosition, PlaneVelocity, PlaneRadius);
}

void FBallPhysics::ApplyImpulse(FBallState& State, const PureMath::FPureVector& PlanePosition, const PureMath::FPureVector& PlaneVelocity) const
{
	const PureMath::FPureVector Normal = ComputeHitNormal(State.Position, PlanePosition, PlaneVelocity);

	const float Approach = PureMath::Dot(PlaneVelocity, Normal);
	const float Impulse = (Approach > 0.f ? Approach * Params.HitTransfer : 0.f) + Params.MinKick;

	State.Velocity = PureMath::Add(State.Velocity, PureMath::Scale(Normal, Impulse));

	const float Speed = PureMath::Length(State.Velocity);
	if (Speed > Params.MaxSpeed)
	{
		State.Velocity = PureMath::Scale(PureMath::Normalized(State.Velocity), Params.MaxSpeed);
	}
}

void FBallPhysics::PushOutOf(FBallState& State, const PureMath::FPureVector& PlanePosition, const PureMath::FPureVector& PlaneVelocity, float PlaneRadius) const
{
	// Mesma normal que ApplyImpulse usou -- a posicao da bola nao mudou entre
	// as duas chamadas, entao ComputeHitNormal devolve o mesmo vetor.
	const PureMath::FPureVector Normal = ComputeHitNormal(State.Position, PlanePosition, PlaneVelocity);
	State.Position = PureMath::Add(PlanePosition, PureMath::Scale(Normal, Params.Radius + PlaneRadius));
}
