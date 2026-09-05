#pragma once

struct FFuelParams
{
	float TankCapacity = 100.f;
	float BoostDrainPerSec = 25.f;          // 4s de boost continuo com o tanque cheio
	float PassiveRegenPerSec = 8.f;         // vazio -> cheio em 12.5s
	float RegenDelayAfterBoostSec = 1.5f;   // pausa a regeneracao logo depois do boost
	float RespawnSeconds = 3.f;             // tempo fora da jogada apos explodir
	float RespawnFuelFraction = 0.5f;       // volta com meio tanque, nao cheio
};

struct FFuelState
{
	float Fuel = 100.f;
	bool bIsDestroyed = false;
	float RespawnTimer = 0.f;
	float TimeSinceBoostSec = 1000.f;   // comeca "ha muito tempo sem boost": regen liberada
};

class FFuelSystem
{
public:
	// Construtor padrao exigido pelo vtable-helper que o UHT gera pra APlanePawn.
	FFuelSystem() : Params() {}
	explicit FFuelSystem(const FFuelParams& InParams) : Params(InParams) {}

	void Update(FFuelState& State, bool bBoostInput, float DeltaSeconds) const;

	// True quando o aviao pode acelerar em boost agora: vivo e com combustivel.
	bool CanBoost(const FFuelState& State) const;

private:
	FFuelParams Params;
};
