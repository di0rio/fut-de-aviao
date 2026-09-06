#pragma once

#include "PureMath.h"
#include "ArenaGeometry.h"

enum class EGoalSide
{
	None,
	West,
	East
};

struct FMatchParams
{
	// Fonte unica da geometria da arena, compartilhada com FBallPhysicsParams --
	// ver ArenaGeometry.h.
	FArenaGeometry Arena;
};

struct FMatchState
{
	int WestScore = 0;   // gols marcados no gol oeste
	int EastScore = 0;
};

class FMatchRules
{
public:
	FMatchRules() : Params() {}
	explicit FMatchRules(const FMatchParams& InParams) : Params(InParams) {}

	// Em qual gol a bola entrou neste instante, se em algum.
	EGoalSide CheckGoal(const PureMath::FPureVector& BallPosition) const;

	void RegisterGoal(FMatchState& State, EGoalSide Side) const;

private:
	FMatchParams Params;
};
