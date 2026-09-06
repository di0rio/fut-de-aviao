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
	BounceAxis(State.Position.X, State.Velocity.X, -Params.ArenaHalfX + R, Params.ArenaHalfX - R, Params.Restitution);
	BounceAxis(State.Position.Y, State.Velocity.Y, -Params.ArenaHalfY + R, Params.ArenaHalfY - R, Params.Restitution);
	BounceAxis(State.Position.Z, State.Velocity.Z, R, Params.ArenaCeilingZ - R, Params.Restitution);
}
