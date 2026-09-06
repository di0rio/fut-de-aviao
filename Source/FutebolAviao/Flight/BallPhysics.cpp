#include "BallPhysics.h"

void FBallPhysics::Update(FBallState& State, float DeltaSeconds) const
{
	State.Velocity.Z -= Params.Gravity * DeltaSeconds;
	State.Position = PureMath::Add(State.Position, PureMath::Scale(State.Velocity, DeltaSeconds));
}
