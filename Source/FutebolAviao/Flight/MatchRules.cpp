#include "MatchRules.h"

EGoalSide FMatchRules::CheckGoal(const PureMath::FPureVector& BallPosition) const
{
	const bool bInsideMouth =
		BallPosition.Y > -Params.GoalHalfWidthY &&
		BallPosition.Y < Params.GoalHalfWidthY &&
		BallPosition.Z > 0.f &&
		BallPosition.Z < Params.GoalHeightZ;

	if (!bInsideMouth)
	{
		return EGoalSide::None;
	}

	if (BallPosition.X < -Params.ArenaHalfX)
	{
		return EGoalSide::West;
	}
	if (BallPosition.X > Params.ArenaHalfX)
	{
		return EGoalSide::East;
	}
	return EGoalSide::None;
}

void FMatchRules::RegisterGoal(FMatchState& State, EGoalSide Side) const
{
	if (Side == EGoalSide::West)
	{
		++State.WestScore;
	}
	else if (Side == EGoalSide::East)
	{
		++State.EastScore;
	}
}
