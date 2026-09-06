#include "../../Source/FutebolAviao/Flight/FuelSystem.h"
#include <cassert>
#include <cstdio>
#include <cmath>

static bool NearlyEqual(float A, float B, float Tolerance = 0.01f)
{
	return std::fabs(A - B) <= Tolerance;
}

static void Test_BoostDrainsFuel()
{
	FFuelParams Params;
	Params.TankCapacity = 100.f;
	Params.BoostDrainPerSec = 25.f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 100.f;

	Fuel.Update(State, /*bBoostActive*/ true, /*DeltaSeconds*/ 1.f);

	assert(NearlyEqual(State.Fuel, 75.f));
	printf("Test_BoostDrainsFuel passed\n");
}

static void Test_NotBoostingRegeneratesFuel()
{
	FFuelParams Params;
	Params.PassiveRegenPerSec = 8.f;
	Params.RegenDelayAfterBoostSec = 1.5f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 50.f;

	Fuel.Update(State, false, 1.f);

	assert(NearlyEqual(State.Fuel, 58.f));
	printf("Test_NotBoostingRegeneratesFuel passed\n");
}

static void Test_RegenIsSuppressedRightAfterBoosting()
{
	FFuelParams Params;
	Params.TankCapacity = 100.f;
	Params.BoostDrainPerSec = 25.f;
	Params.PassiveRegenPerSec = 8.f;
	Params.RegenDelayAfterBoostSec = 1.5f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 50.f;

	Fuel.Update(State, true, 1.f);    // gasta 25 -> 25 de combustivel, zera o relogio
	Fuel.Update(State, false, 1.f);   // 1s < 1.5s de delay: nao pode regenerar ainda

	assert(NearlyEqual(State.Fuel, 25.f));
	printf("Test_RegenIsSuppressedRightAfterBoosting passed\n");
}

static void Test_FuelNeverExceedsTankCapacity()
{
	FFuelParams Params;
	Params.TankCapacity = 100.f;
	Params.PassiveRegenPerSec = 500.f;
	Params.RegenDelayAfterBoostSec = 0.f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 90.f;

	Fuel.Update(State, false, 1.f);

	assert(NearlyEqual(State.Fuel, 100.f));
	printf("Test_FuelNeverExceedsTankCapacity passed\n");
}

static void Test_EmptyTankDestroysThePlane()
{
	FFuelParams Params;
	Params.BoostDrainPerSec = 25.f;
	Params.RespawnSeconds = 3.f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 10.f;

	Fuel.Update(State, true, 1.f);   // pede 25 de dreno com so 10 no tanque

	assert(NearlyEqual(State.Fuel, 0.f));
	assert(State.bIsDestroyed);
	assert(NearlyEqual(State.RespawnTimer, 3.f));
	printf("Test_EmptyTankDestroysThePlane passed\n");
}

static void Test_PlaneRespawnsWithHalfTankAfterTimer()
{
	FFuelParams Params;
	Params.TankCapacity = 100.f;
	Params.RespawnSeconds = 3.f;
	Params.RespawnFuelFraction = 0.5f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 0.f;
	State.bIsDestroyed = true;
	State.RespawnTimer = 3.f;

	Fuel.Update(State, false, 1.f);
	assert(State.bIsDestroyed);              // 2s restantes: ainda fora da jogada

	Fuel.Update(State, false, 1.f);
	Fuel.Update(State, false, 1.f);

	assert(!State.bIsDestroyed);
	assert(NearlyEqual(State.Fuel, 50.f));
	printf("Test_PlaneRespawnsWithHalfTankAfterTimer passed\n");
}

static void Test_CanBoostIsFalseWhileDestroyedOrEmpty()
{
	FFuelSystem Fuel((FFuelParams()));

	FFuelState Alive;
	Alive.Fuel = 10.f;
	Alive.bIsDestroyed = false;
	assert(Fuel.CanBoost(Alive));

	FFuelState Empty;
	Empty.Fuel = 0.f;
	Empty.bIsDestroyed = false;
	assert(!Fuel.CanBoost(Empty));

	FFuelState Destroyed;
	Destroyed.Fuel = 50.f;
	Destroyed.bIsDestroyed = true;
	assert(!Fuel.CanBoost(Destroyed));

	printf("Test_CanBoostIsFalseWhileDestroyedOrEmpty passed\n");
}

static void Test_FuelDoesNotRegenerateWhileDestroyed()
{
	// O teste de respawn existente nunca checa o combustivel durante os frames
	// destruidos, e o valor final e escrito pela atribuicao do respawn, o que
	// mascara qualquer vazamento de regen enquanto o aviao esta fora da jogada.
	FFuelParams Params;
	Params.RespawnSeconds = 100.f;
	Params.PassiveRegenPerSec = 500.f;
	Params.RegenDelayAfterBoostSec = 0.f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 0.f;
	State.bIsDestroyed = true;
	State.RespawnTimer = 100.f;

	Fuel.Update(State, false, 1.f);
	Fuel.Update(State, false, 1.f);
	Fuel.Update(State, false, 1.f);

	assert(State.bIsDestroyed);
	assert(NearlyEqual(State.Fuel, 0.f));
	printf("Test_FuelDoesNotRegenerateWhileDestroyed passed\n");
}

static void Test_RespawnResetsPostBoostRegenDelay()
{
	// TimeSinceBoostSec precisa voltar a 0 no respawn: sem isso o regen nao
	// respeitaria o RegenDelayAfterBoostSec logo apos o aviao voltar a jogada.
	//
	// A fixture entra no estado destruido DIRETAMENTE, sem boostar antes: se
	// chegasse la boostando, o proprio branch de boost ja teria zerado
	// TimeSinceBoostSec, e a assercao final passaria mesmo que a linha que
	// zera o relogio no respawn fosse apagada. Comecando com o default de
	// 1000.f (definido em FFuelState) e nunca tocando em TimeSinceBoostSec
	// fora do respawn, so o codigo do respawn pode zera-lo.
	FFuelParams Params;
	Params.TankCapacity = 100.f;
	Params.RespawnSeconds = 3.f;
	Params.RespawnFuelFraction = 0.5f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 0.f;
	State.bIsDestroyed = true;
	State.RespawnTimer = 3.f;

	Fuel.Update(State, false, 3.f);  // deixa o timer de respawn zerar

	assert(!State.bIsDestroyed);
	assert(NearlyEqual(State.TimeSinceBoostSec, 0.f));
	printf("Test_RespawnResetsPostBoostRegenDelay passed\n");
}

static void Test_ResolveBoostDoesNotReengageAfterRespawnWhenInputCleared()
{
	// ATENCAO: isto NAO e um teste de regressao do death loop (Fix 1). O fix de
	// verdade e "bBoostInput = false;" no bloco de respawn de APlanePawn::Tick,
	// em PlanePawn.cpp - e essa linha vive na camada de Unreal. As suites
	// standalone deste repositorio (Tools/*) so linkam FlightPhysics.cpp e
	// FuelSystem.cpp, ambos sem headers da engine; elas nao conseguem
	// instanciar AActor/APawn, entao nao ha como este arquivo tocar
	// APlanePawn::Tick nem provar que aquela linha existe ou continua existindo.
	// Apagar "bBoostInput = false;" de PlanePawn.cpp deixa esta suite inteira
	// verde.
	//
	// O que este teste de fato verifica: que FFuelSystem::ResolveBoost, dado um
	// pedido de boost que ja foi "limpo" (bBoostRequested == false) apos um
	// ciclo destruir/respawnar, nao reengata o boost sozinho - e que, se o
	// pedido NAO fosse limpo, ResolveBoost reengataria (mostrando por que o
	// pawn precisa limpar). Isto cobre a aritmetica pura de ResolveBoost/
	// CanBoost; a responsabilidade do pawn de realmente zerar bBoostInput
	// continua sem cobertura automatizada e so pode ser verificada manualmente
	// ou por um teste de Automation da Unreal (fora do escopo aqui).
	FFuelParams Params;
	Params.TankCapacity = 50.f;
	Params.BoostDrainPerSec = 25.f;
	Params.RespawnSeconds = 3.f;
	Params.RespawnFuelFraction = 1.f;
	FFuelSystem Fuel(Params);
	FFuelState State;
	State.Fuel = 50.f;

	// Boost segurado ate destruir (2s de dreno a 25/s esvazia os 50 de tanque).
	bool bBoostRequested = true;
	for (int i = 0; i < 2; ++i)
	{
		const bool bBoostActive = Fuel.ResolveBoost(State, bBoostRequested);
		Fuel.Update(State, bBoostActive, 1.f);
	}
	assert(State.bIsDestroyed);

	// Atravessa a janela de respawn com o input de boost ainda "segurado".
	while (State.bIsDestroyed)
	{
		const bool bBoostActive = Fuel.ResolveBoost(State, bBoostRequested);
		Fuel.Update(State, bBoostActive, 1.f);
	}
	assert(!State.bIsDestroyed);
	assert(NearlyEqual(State.Fuel, 50.f));

	// METADE DO BUG: se o pawn nao limpasse bBoostInput (bBoostRequested continua
	// true aqui, como na versao com defeito), ResolveBoost volta a engatar o boost
	// no primeiro frame pos-respawn porque CanBoost ja e true de novo.
	assert(Fuel.ResolveBoost(State, bBoostRequested));

	// METADE DO FIX: o pawn agora zera bBoostInput no respawn (Fix 1). Com o
	// pedido limpo, o boost nao reengata sozinho e o combustivel nao e drenado
	// de novo -> sem loop de morte.
	bBoostRequested = false;
	const bool bBoostActiveAfterFix = Fuel.ResolveBoost(State, bBoostRequested);
	assert(!bBoostActiveAfterFix);
	Fuel.Update(State, bBoostActiveAfterFix, 1.f);

	assert(!State.bIsDestroyed);
	assert(NearlyEqual(State.Fuel, 50.f));
	printf("Test_ResolveBoostDoesNotReengageAfterRespawnWhenInputCleared passed\n");
}

int main()
{
	Test_BoostDrainsFuel();
	Test_NotBoostingRegeneratesFuel();
	Test_RegenIsSuppressedRightAfterBoosting();
	Test_FuelNeverExceedsTankCapacity();
	Test_EmptyTankDestroysThePlane();
	Test_PlaneRespawnsWithHalfTankAfterTimer();
	Test_CanBoostIsFalseWhileDestroyedOrEmpty();
	Test_FuelDoesNotRegenerateWhileDestroyed();
	Test_RespawnResetsPostBoostRegenDelay();
	Test_ResolveBoostDoesNotReengageAfterRespawnWhenInputCleared();
	printf("All tests passed\n");
	return 0;
}
