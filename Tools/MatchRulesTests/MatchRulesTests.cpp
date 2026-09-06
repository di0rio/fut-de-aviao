#include "../../Source/FutebolAviao/Flight/MatchRules.h"
#include <cassert>
#include <cstdio>

static void Test_BallInsideWestGoalCountsForEast()
{
	FMatchParams Params;
	Params.Arena.ArenaHalfX = 10000.f;
	Params.Arena.GoalHalfWidthY = 1500.f;
	Params.Arena.GoalHeightZ = 2000.f;
	FMatchRules Rules(Params);

	PureMath::FPureVector Ball;
	Ball.X = -10050.f;   // passou da linha oeste
	Ball.Y = 0.f;
	Ball.Z = 800.f;

	assert(Rules.CheckGoal(Ball) == EGoalSide::West);
	printf("Test_BallInsideWestGoalCountsForEast passed\n");
}

static void Test_BallInsideEastGoalCountsForWest()
{
	FMatchParams Params;
	Params.Arena.ArenaHalfX = 10000.f;
	Params.Arena.GoalHalfWidthY = 1500.f;
	Params.Arena.GoalHeightZ = 2000.f;
	FMatchRules Rules(Params);

	PureMath::FPureVector Ball;
	Ball.X = 10050.f;   // passou da linha leste
	Ball.Y = 0.f;
	Ball.Z = 800.f;

	assert(Rules.CheckGoal(Ball) == EGoalSide::East);
	printf("Test_BallInsideEastGoalCountsForWest passed\n");
}

static void Test_BallPastTheLineButOutsideTheMouthIsNotAGoal()
{
	FMatchParams Params;
	Params.Arena.ArenaHalfX = 10000.f;
	Params.Arena.GoalHalfWidthY = 1500.f;
	Params.Arena.GoalHeightZ = 2000.f;
	FMatchRules Rules(Params);

	PureMath::FPureVector TooWide;
	TooWide.X = -10050.f;
	TooWide.Y = 4000.f;    // passou da linha, mas longe do gol
	TooWide.Z = 800.f;
	assert(Rules.CheckGoal(TooWide) == EGoalSide::None);

	PureMath::FPureVector TooHigh;
	TooHigh.X = -10050.f;
	TooHigh.Y = 0.f;
	TooHigh.Z = 3500.f;    // por cima do travessao
	assert(Rules.CheckGoal(TooHigh) == EGoalSide::None);

	printf("Test_BallPastTheLineButOutsideTheMouthIsNotAGoal passed\n");
}

static void Test_RegisteringGoalsIncrementsTheRightSide()
{
	FMatchRules Rules((FMatchParams()));
	FMatchState State;

	Rules.RegisterGoal(State, EGoalSide::West);
	Rules.RegisterGoal(State, EGoalSide::West);
	Rules.RegisterGoal(State, EGoalSide::East);
	Rules.RegisterGoal(State, EGoalSide::None);   // nao conta nada

	assert(State.WestScore == 2);
	assert(State.EastScore == 1);
	printf("Test_RegisteringGoalsIncrementsTheRightSide passed\n");
}

int main()
{
	Test_BallInsideWestGoalCountsForEast();
	Test_BallInsideEastGoalCountsForWest();
	Test_BallPastTheLineButOutsideTheMouthIsNotAGoal();
	Test_RegisteringGoalsIncrementsTheRightSide();
	printf("All tests passed\n");
	return 0;
}
