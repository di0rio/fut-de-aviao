# Fase 2 — Combustível + Boost — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** O avião ganha um tanque de combustível, um boost que consome esse tanque, regeneração passiva, e explosão + respawn quando o tanque zera — validando se o custo/benefício do boost cria decisões interessantes antes de existir bola.

**Architecture:** Mesmo padrão que provou funcionar na Fase 1 — a regra de combustível vira uma classe C++ pura (`FFuelSystem`), sem nenhuma dependência da Unreal, com um executável de teste standalone. `APlanePawn` continua sendo um adapter fino: chama `FFuelSystem::Update` a cada `Tick` e traduz o resultado em efeito visível (esconder o avião durante o respawn, teleportar de volta ao ponto inicial). O boost entra em `FFlightPhysics` como um par de parâmetros extra (`BoostMaxSpeed`/`BoostAcceleration`) mais um argumento `bBoostActive` no `Update` — assim quem decide *se* pode dar boost é o sistema de combustível, e quem decide *o que* o boost faz é a física.

**Tech Stack:** Unreal Engine 5.8, C++17, sistema de Input clássico (Axis/Action Mappings em `DefaultInput.ini`).

## Global Constraints

- Nome do projeto/módulo Unreal: `FutebolAviao` (o repositório git chama-se `fut-de-aviao`; o módulo C++ **não** muda).
- Engine `5.8`, instalação local em `C:\Program Files\Epic Games\UE_5.8`. Win64 apenas.
- Todo o código desta fase é C++, sem Blueprints — cada task é executável por edição de arquivo + compilação por linha de comando.
- **Toda regra numérica de combustível fica numa classe pura, testável sem a engine.** A spec é explícita: testes automatizados fazem sentido "só para lógica isolada (cálculo de consumo/regeneração de combustível...)" — é exatamente esta fase.
- **Escopo desta fase:** tanque, consumo no boost, regeneração passiva, explosão/respawn ao zerar. **Fora do escopo:** pickups de combustível espalhados pelo campo (isso é Fase 4, junto com o 2v2 local), bola, gol, rede/replicação, HUD com arte.
- Servidor autoritativo é requisito de arquitetura da spec, mas só entra de fato na Fase 5. Nesta fase o código roda local; **não** introduzir `Replicated`/RPC ainda — só manter a lógica em classes puras, que é o que torna a migração barata depois.
- Antes de compilar pela linha de comando, fechar `UnrealEditor.exe` e `LiveCodingConsole.exe`.

**Comando de teste standalone do combustível** (o aviso `vswhere.exe not recognized` é ruído esperado e não indica falha):

```bash
cmd /c '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cd /d "C:\Users\cauad\Desktop\dev\jogao" && cl /nologo /EHsc /std:c++17 /Fe:Tools\FuelSystemTests\FuelSystemTests.exe Tools\FuelSystemTests\FuelSystemTests.cpp Source\FutebolAviao\Flight\FuelSystem.cpp && Tools\FuelSystemTests\FuelSystemTests.exe'
```

**Comando de teste standalone da física de voo:**

```bash
cmd /c '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cd /d "C:\Users\cauad\Desktop\dev\jogao" && cl /nologo /EHsc /std:c++17 /Fe:Tools\FlightPhysicsTests\FlightPhysicsTests.exe Tools\FlightPhysicsTests\FlightPhysicsTests.cpp Source\FutebolAviao\Flight\FlightPhysics.cpp && Tools\FlightPhysicsTests\FlightPhysicsTests.exe'
```

**Comando de build do editor:**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FutebolAviaoEditor Win64 Development -Project="C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -WaitMutex
```

---

### Task 1: Sistema de combustível puro (sem dependência da engine)

**Files:**
- Create: `Source/FutebolAviao/Flight/FuelSystem.h`
- Create: `Source/FutebolAviao/Flight/FuelSystem.cpp`
- Test: `Tools/FuelSystemTests/FuelSystemTests.cpp`

**Interfaces:**
- Produces: `struct FFuelParams` (campos `TankCapacity`, `BoostDrainPerSec`, `PassiveRegenPerSec`, `RegenDelayAfterBoostSec`, `RespawnSeconds`, `RespawnFuelFraction`, todos `float`); `struct FFuelState` (campos `Fuel` `float`, `bIsDestroyed` `bool`, `RespawnTimer` `float`, `TimeSinceBoostSec` `float`); `class FFuelSystem` com `FFuelSystem()`, `explicit FFuelSystem(const FFuelParams& InParams)`, `void Update(FFuelState& State, bool bBoostInput, float DeltaSeconds) const` e `bool CanBoost(const FFuelState& State) const`. A Task 3 consome exatamente essas assinaturas.
- O construtor padrão existe pelo mesmo motivo que em `FFlightPhysics`: o UHT gera um vtable-helper constructor para `APlanePawn` que exige todo membro C++ comum default-construtível.

- [ ] **Step 1: Escrever o primeiro teste (vai falhar — a classe não existe)**

Criar `Tools/FuelSystemTests/FuelSystemTests.cpp`:

```cpp
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

	Fuel.Update(State, /*bBoostInput*/ true, /*DeltaSeconds*/ 1.f);

	assert(NearlyEqual(State.Fuel, 75.f));
	printf("Test_BoostDrainsFuel passed\n");
}

int main()
{
	Test_BoostDrainsFuel();
	printf("All tests passed\n");
	return 0;
}
```

- [ ] **Step 2: Rodar e confirmar que falha**

Rodar o comando de teste standalone do combustível.
Esperado: erro de compilação `Cannot open include file: '../../Source/FutebolAviao/Flight/FuelSystem.h'`. É a falha certa — o arquivo ainda não existe.

- [ ] **Step 3: Criar o header com os defaults**

Criar `Source/FutebolAviao/Flight/FuelSystem.h`:

```cpp
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
```

Criar `Source/FutebolAviao/Flight/FuelSystem.cpp` com o mínimo pro teste passar:

```cpp
#include "FuelSystem.h"

void FFuelSystem::Update(FFuelState& State, bool bBoostInput, float DeltaSeconds) const
{
	if (bBoostInput)
	{
		State.Fuel -= Params.BoostDrainPerSec * DeltaSeconds;
	}
}

bool FFuelSystem::CanBoost(const FFuelState& State) const
{
	return !State.bIsDestroyed && State.Fuel > 0.f;
}
```

- [ ] **Step 4: Rodar e confirmar que passa**

Esperado: `Test_BoostDrainsFuel passed` / `All tests passed`.

- [ ] **Step 5: Commit**

```bash
git add Source/FutebolAviao/Flight/FuelSystem.h Source/FutebolAviao/Flight/FuelSystem.cpp Tools/FuelSystemTests/FuelSystemTests.cpp
git commit -m "feat: add fuel system with boost drain"
```

- [ ] **Step 6: Teste da regeneração passiva (vai falhar)**

Inserir antes de `int main()` em `Tools/FuelSystemTests/FuelSystemTests.cpp`:

```cpp
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
```

E registrar em `main()`, logo depois de `Test_BoostDrainsFuel();`:

```cpp
	Test_NotBoostingRegeneratesFuel();
```

- [ ] **Step 7: Rodar e confirmar que falha**

Esperado: `Assertion failed: NearlyEqual(State.Fuel, 58.f)` — o `Update` ainda não regenera nada, então `Fuel` continua 50.

- [ ] **Step 8: Implementar a regeneração**

Substituir o corpo de `FFuelSystem::Update` em `FuelSystem.cpp`:

```cpp
void FFuelSystem::Update(FFuelState& State, bool bBoostInput, float DeltaSeconds) const
{
	if (bBoostInput)
	{
		State.Fuel -= Params.BoostDrainPerSec * DeltaSeconds;
		State.TimeSinceBoostSec = 0.f;
	}
	else
	{
		State.TimeSinceBoostSec += DeltaSeconds;
		if (State.TimeSinceBoostSec >= Params.RegenDelayAfterBoostSec)
		{
			State.Fuel += Params.PassiveRegenPerSec * DeltaSeconds;
		}
	}
}
```

Atenção: o `FFuelState` default começa com `TimeSinceBoostSec = 1000.f`, então o primeiro `Update` sem boost já regenera — que é o que o teste espera.

- [ ] **Step 9: Rodar e confirmar que os dois testes passam**

Esperado: `Test_BoostDrainsFuel passed`, `Test_NotBoostingRegeneratesFuel passed`, `All tests passed`.

- [ ] **Step 10: Teste do delay de regeneração**

Inserir antes de `int main()`:

```cpp
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
```

E em `main()`, depois de `Test_NotBoostingRegeneratesFuel();`:

```cpp
	Test_RegenIsSuppressedRightAfterBoosting();
```

- [ ] **Step 11: Rodar e confirmar que passa**

Este teste passa direto, porque o Step 8 já implementou o delay. **Isso é um teste escrito depois da implementação** — não conta como ciclo TDD, mas vale manter: trava o comportamento contra regressão. Para rigor estrito, apagar o tratamento de `TimeSinceBoostSec` do Step 8, ver este teste falhar, e reimplementar.

- [ ] **Step 12: Commit**

```bash
git add Source/FutebolAviao/Flight/FuelSystem.cpp Tools/FuelSystemTests/FuelSystemTests.cpp
git commit -m "feat: add passive fuel regeneration with post-boost delay"
```

- [ ] **Step 13: Teste do teto do tanque (vai falhar)**

Inserir antes de `int main()`:

```cpp
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
```

E em `main()`, depois de `Test_RegenIsSuppressedRightAfterBoosting();`:

```cpp
	Test_FuelNeverExceedsTankCapacity();
```

- [ ] **Step 14: Rodar e confirmar que falha**

Esperado: `Assertion failed: NearlyEqual(State.Fuel, 100.f)` — sem clamp, `Fuel` vira 590.

- [ ] **Step 15: Adicionar o clamp**

No topo de `FuelSystem.cpp`, logo depois do `#include`, adicionar o helper:

```cpp
namespace
{
	float ClampValue(float Value, float Min, float Max)
	{
		if (Value < Min) return Min;
		if (Value > Max) return Max;
		return Value;
	}
}
```

E no fim de `FFuelSystem::Update`, depois do `if/else` de boost:

```cpp
	State.Fuel = ClampValue(State.Fuel, 0.f, Params.TankCapacity);
```

- [ ] **Step 16: Rodar e confirmar que os quatro testes passam**

- [ ] **Step 17: Commit**

```bash
git add Source/FutebolAviao/Flight/FuelSystem.cpp Tools/FuelSystemTests/FuelSystemTests.cpp
git commit -m "feat: clamp fuel to tank capacity"
```

- [ ] **Step 18: Teste da explosão ao zerar (vai falhar)**

Inserir antes de `int main()`:

```cpp
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
```

E em `main()`, depois de `Test_FuelNeverExceedsTankCapacity();`:

```cpp
	Test_EmptyTankDestroysThePlane();
```

- [ ] **Step 19: Rodar e confirmar que falha**

Esperado: `Assertion failed: State.bIsDestroyed` — o `Fuel` já chega a 0 pelo clamp, mas nada marca o avião como destruído.

- [ ] **Step 20: Implementar a explosão**

Em `FuelSystem.cpp`, logo depois da linha do clamp:

```cpp
	if (State.Fuel <= 0.f && !State.bIsDestroyed)
	{
		State.bIsDestroyed = true;
		State.RespawnTimer = Params.RespawnSeconds;
	}
```

- [ ] **Step 21: Rodar e confirmar que os cinco testes passam**

- [ ] **Step 22: Commit**

```bash
git add Source/FutebolAviao/Flight/FuelSystem.cpp Tools/FuelSystemTests/FuelSystemTests.cpp
git commit -m "feat: destroy the plane when the tank runs dry"
```

- [ ] **Step 23: Teste do respawn (vai falhar)**

Inserir antes de `int main()`:

```cpp
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
```

E em `main()`, depois de `Test_EmptyTankDestroysThePlane();`:

```cpp
	Test_PlaneRespawnsWithHalfTankAfterTimer();
```

- [ ] **Step 24: Rodar e confirmar que falha**

Esperado: `Assertion failed: State.bIsDestroyed` na primeira asserção — hoje o `Update` regenera combustível mesmo destruído, sai do 0, e nada decrementa o timer.

- [ ] **Step 25: Implementar o respawn**

Substituir o corpo inteiro de `FFuelSystem::Update` por:

```cpp
void FFuelSystem::Update(FFuelState& State, bool bBoostInput, float DeltaSeconds) const
{
	// Destruido: nao consome, nao regenera, so espera o respawn.
	if (State.bIsDestroyed)
	{
		State.RespawnTimer -= DeltaSeconds;
		if (State.RespawnTimer <= 0.f)
		{
			State.bIsDestroyed = false;
			State.RespawnTimer = 0.f;
			State.Fuel = Params.TankCapacity * Params.RespawnFuelFraction;
			State.TimeSinceBoostSec = 0.f;
		}
		return;
	}

	if (bBoostInput)
	{
		State.Fuel -= Params.BoostDrainPerSec * DeltaSeconds;
		State.TimeSinceBoostSec = 0.f;
	}
	else
	{
		State.TimeSinceBoostSec += DeltaSeconds;
		if (State.TimeSinceBoostSec >= Params.RegenDelayAfterBoostSec)
		{
			State.Fuel += Params.PassiveRegenPerSec * DeltaSeconds;
		}
	}

	State.Fuel = ClampValue(State.Fuel, 0.f, Params.TankCapacity);

	if (State.Fuel <= 0.f && !State.bIsDestroyed)
	{
		State.bIsDestroyed = true;
		State.RespawnTimer = Params.RespawnSeconds;
	}
}
```

- [ ] **Step 26: Rodar e confirmar que os seis testes passam**

- [ ] **Step 27: Teste de que boost não funciona destruído nem vazio**

Inserir antes de `int main()`:

```cpp
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
```

E em `main()`, depois de `Test_PlaneRespawnsWithHalfTankAfterTimer();`:

```cpp
	Test_CanBoostIsFalseWhileDestroyedOrEmpty();
```

- [ ] **Step 28: Rodar e confirmar que os sete testes passam**

Passa direto — `CanBoost` já foi escrito no Step 3. Vale manter pelo mesmo motivo do Step 11: trava o contrato que a Task 3 vai consumir.

- [ ] **Step 29: Commit**

```bash
git add Source/FutebolAviao/Flight/FuelSystem.cpp Tools/FuelSystemTests/FuelSystemTests.cpp
git commit -m "feat: respawn the plane with half a tank after the penalty timer"
```

---

### Task 2: Boost na física de voo

**Files:**
- Modify: `Source/FutebolAviao/Flight/FlightPhysics.h`
- Modify: `Source/FutebolAviao/Flight/FlightPhysics.cpp`
- Modify: `Tools/FlightPhysicsTests/FlightPhysicsTests.cpp`

**Interfaces:**
- Consumes: nada da Task 1 — as duas classes são independentes de propósito; quem as junta é a Task 3.
- Produces: `FFlightPhysicsParams` ganha `BoostMaxSpeed` e `BoostAcceleration` (`float`). `FFlightPhysics::Update` passa a ter a assinatura `void Update(FFlightPhysicsState& State, float ThrottleInput, float PitchInput, float YawInput, float RollInput, bool bBoostActive, float DeltaSeconds) const` — **o `bBoostActive` entra antes do `DeltaSeconds`**. A Task 3 chama exatamente essa forma.

- [ ] **Step 1: Escrever o teste do boost (vai falhar)**

Inserir antes de `int main()` em `Tools/FlightPhysicsTests/FlightPhysicsTests.cpp`:

```cpp
static void Test_BoostAcceleratesPastNormalMaxSpeed()
{
	FFlightPhysicsParams Params;
	Params.MinSpeed = 0.f;
	Params.MaxSpeed = 6000.f;
	Params.BoostMaxSpeed = 9000.f;
	Params.BoostAcceleration = 5000.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;
	State.Speed = 6000.f;

	Physics.Update(State, 1.f, 0.f, 0.f, 0.f, /*bBoostActive*/ true, 1.f);

	assert(NearlyEqual(State.Speed, 9000.f)); // 6000 + 5000 = 11000, clampado no teto do boost
	printf("Test_BoostAcceleratesPastNormalMaxSpeed passed\n");
}
```

E em `main()`, depois de `Test_YawInputWrapsAround360();`:

```cpp
	Test_BoostAcceleratesPastNormalMaxSpeed();
```

- [ ] **Step 2: Rodar e confirmar que falha**

Rodar o comando de teste standalone da física de voo.
Esperado: erros de compilação — `'BoostMaxSpeed': is not a member of 'FFlightPhysicsParams'` e `'Update': function does not take 7 arguments`.

- [ ] **Step 3: Adicionar os parâmetros de boost**

Em `Source/FutebolAviao/Flight/FlightPhysics.h`, dentro de `FFlightPhysicsParams`, logo depois da linha do `MaxSpeed`:

```cpp
	float BoostMaxSpeed = 9000.f;    // teto so alcancavel em boost (1.5x a maxima normal)
	float BoostAcceleration = 5000.f;
```

E trocar a declaração do `Update` na mesma classe por:

```cpp
	void Update(FFlightPhysicsState& State, float ThrottleInput, float PitchInput, float YawInput, float RollInput, bool bBoostActive, float DeltaSeconds) const;
```

- [ ] **Step 4: Implementar o boost**

Em `Source/FutebolAviao/Flight/FlightPhysics.cpp`, trocar a assinatura do `Update` pela nova, e substituir o bloco de aceleração/clamp (do `if (ThrottleInput > 0.f)` até a linha `State.Speed = ClampValue(...)` inclusive) por:

```cpp
	if (bBoostActive)
	{
		State.Speed += Params.BoostAcceleration * DeltaSeconds;
	}
	else if (ThrottleInput > 0.f)
	{
		State.Speed += Params.Acceleration * ThrottleInput * DeltaSeconds;
	}
	else if (ThrottleInput < 0.f)
	{
		State.Speed += Params.Deceleration * ThrottleInput * DeltaSeconds;
	}
	else if (State.Speed > 0.f)
	{
		State.Speed -= Params.Drag * DeltaSeconds;
	}

	// Fora do boost o teto e a maxima normal; o excesso ganho em boost e cortado.
	const float SpeedCeiling = bBoostActive ? Params.BoostMaxSpeed : Params.MaxSpeed;
	State.Speed = ClampValue(State.Speed, Params.MinSpeed, SpeedCeiling);
```

Nota: ao soltar o boost acima de `MaxSpeed`, o clamp derruba a velocidade pro teto normal no mesmo frame. Isso é intencional e simples; se no playtest ficar abrupto, a Task 4 troca por um decaimento gradual.

- [ ] **Step 5: Corrigir as chamadas existentes**

Os seis testes antigos em `Tools/FlightPhysicsTests/FlightPhysicsTests.cpp` chamam `Update` com 6 argumentos. Em cada um, inserir `false,` antes do último argumento (o `DeltaSeconds`). Exemplo — a chamada em `Test_ThrottleAcceleratesSpeed` passa de:

```cpp
	Physics.Update(State, /*Throttle*/ 1.f, 0.f, 0.f, 0.f, /*DeltaSeconds*/ 1.f);
```

para:

```cpp
	Physics.Update(State, /*Throttle*/ 1.f, 0.f, 0.f, 0.f, /*bBoostActive*/ false, /*DeltaSeconds*/ 1.f);
```

Fazer o mesmo nas chamadas de `Test_SpeedClampsToMaxSpeed`, `Test_NoThrottleAppliesDrag`, `Test_SpeedNeverGoesNegativeFromDrag`, `Test_PitchInputRotatesNoseAndClamps` e `Test_YawInputWrapsAround360`.

- [ ] **Step 6: Rodar e confirmar que os sete testes passam**

Esperado: as seis linhas antigas mais `Test_BoostAcceleratesPastNormalMaxSpeed passed` e `All tests passed`.

- [ ] **Step 7: Commit**

```bash
git add Source/FutebolAviao/Flight/FlightPhysics.h Source/FutebolAviao/Flight/FlightPhysics.cpp Tools/FlightPhysicsTests/FlightPhysicsTests.cpp
git commit -m "feat: add boost speed ceiling and acceleration to flight physics"
```

---

### Task 3: Ligar combustível e boost no APlanePawn

**Files:**
- Modify: `Source/FutebolAviao/Flight/PlanePawn.h`
- Modify: `Source/FutebolAviao/Flight/PlanePawn.cpp`
- Modify: `Config/DefaultInput.ini`

**Interfaces:**
- Consumes: `FFuelSystem`, `FFuelParams`, `FFuelState` da Task 1; a nova assinatura de `FFlightPhysics::Update` (com `bBoostActive`) da Task 2.
- Produces: nada consumido por tasks futuras desta fase — a Task 4 é playtest.

- [ ] **Step 1: Adicionar o Action Mapping do boost**

Em `Config/DefaultInput.ini`, inserir logo depois da última linha `+AxisMappings=(AxisName="Yaw",...)`:

```ini
+ActionMappings=(ActionName="Boost",Key=LeftShift)
+ActionMappings=(ActionName="Boost",Key=Gamepad_RightTrigger)
```

- [ ] **Step 2: Declarar o estado de combustível no pawn**

Em `Source/FutebolAviao/Flight/PlanePawn.h`, trocar o `#include "FlightPhysics.h"` por:

```cpp
#include "FlightPhysics.h"
#include "FuelSystem.h"
```

Adicionar, junto dos outros `Handle*Input`, os dois handlers do boost:

```cpp
	void HandleBoostPressed();
	void HandleBoostReleased();
```

E na seção `private:`, logo depois de `FFlightPhysicsState FlightState;`:

```cpp
	FFuelSystem FuelSystem;
	FFuelState FuelState;

	// Transform capturado no BeginPlay: e pra ca que o aviao volta ao respawnar.
	FTransform SpawnTransform;

	bool bBoostInput = false;
	bool bWasDestroyed = false;
```

- [ ] **Step 3: Guardar o ponto de respawn**

Em `Source/FutebolAviao/Flight/PlanePawn.cpp`, substituir o corpo de `BeginPlay`:

```cpp
void APlanePawn::BeginPlay()
{
	Super::BeginPlay();
	SpawnTransform = GetActorTransform();
}
```

- [ ] **Step 4: Ligar o input de boost**

Em `SetupPlayerInputComponent`, depois do último `BindAxis`:

```cpp
	PlayerInputComponent->BindAction("Boost", IE_Pressed, this, &APlanePawn::HandleBoostPressed);
	PlayerInputComponent->BindAction("Boost", IE_Released, this, &APlanePawn::HandleBoostReleased);
```

E no fim do arquivo, junto dos outros handlers:

```cpp
void APlanePawn::HandleBoostPressed()
{
	bBoostInput = true;
}

void APlanePawn::HandleBoostReleased()
{
	bBoostInput = false;
}
```

- [ ] **Step 5: Consumir combustível e reagir à destruição no Tick**

Adicionar no topo de `PlanePawn.cpp`, junto dos outros includes:

```cpp
#include "Engine/Engine.h"
```

E substituir o corpo inteiro de `APlanePawn::Tick`:

```cpp
void APlanePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const bool bBoostActive = bBoostInput && FuelSystem.CanBoost(FuelState);
	FuelSystem.Update(FuelState, bBoostActive, DeltaSeconds);

	// Explodiu neste frame: some da arena e para de voar.
	if (FuelState.bIsDestroyed && !bWasDestroyed)
	{
		bWasDestroyed = true;
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}

	// Respawnou neste frame: volta ao ponto inicial, parado e visivel.
	if (!FuelState.bIsDestroyed && bWasDestroyed)
	{
		bWasDestroyed = false;
		SetActorTransform(SpawnTransform);
		FlightState = FFlightPhysicsState();
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
	}

	if (FuelState.bIsDestroyed)
	{
		return; // fora da jogada: sem fisica de voo enquanto o timer roda
	}

	FlightPhysics.Update(FlightState, ThrottleInput, PitchInput, YawInput, RollInput, bBoostActive, DeltaSeconds);

	const FRotator NewRotation(FlightState.PitchDeg, FlightState.YawDeg, FlightState.RollDeg);
	SetActorRotation(NewRotation);

	const FVector Forward = NewRotation.Vector();
	AddActorWorldOffset(Forward * FlightState.Speed * DeltaSeconds, true);

	// Sem HUD ainda: o combustivel aparece como texto de debug pra dar pra jogar a Task 4.
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow,
			FString::Printf(TEXT("Fuel %.0f  Speed %.0f%s"), FuelState.Fuel, FlightState.Speed, bBoostActive ? TEXT("  BOOST") : TEXT("")));
	}
}
```

- [ ] **Step 6: Compilar o editor**

Fechar `UnrealEditor.exe` e `LiveCodingConsole.exe`, depois rodar o comando de build do editor das Global Constraints.
Esperado: `Result: Succeeded`.

- [ ] **Step 7: Rodar os dois conjuntos de testes standalone**

Esperado: `All tests passed` nos dois. Esta task não muda regra nenhuma — só liga as classes na engine —, então nenhum teste novo é esperado aqui; rodar os existentes é a verificação de que nada regrediu.

- [ ] **Step 8: Commit**

```bash
git add Source/FutebolAviao/Flight/PlanePawn.h Source/FutebolAviao/Flight/PlanePawn.cpp Config/DefaultInput.ini
git commit -m "feat: wire fuel and boost into PlanePawn with explode/respawn"
```

---

### Task 4: Playtest e tuning

**Files:**
- Modify (só se o playtest pedir): `Source/FutebolAviao/Flight/FuelSystem.h`, `Source/FutebolAviao/Flight/FlightPhysics.h`

**Interfaces:**
- Consumes: tudo das Tasks 1 a 3.
- Produces: entrega final da Fase 2.

- [ ] **Step 1: Abrir o editor e jogar**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject"
```

Apertar Play (Alt+P). O texto amarelo no canto mostra `Fuel`, `Speed` e `BOOST`.

- [ ] **Step 2: Conferir cada comportamento**

- Segurar `LeftShift` acelera acima da máxima normal e o `Fuel` cai.
- Soltar o boost: o `Fuel` fica parado por ~1.5s e só então volta a subir.
- Segurar o boost até zerar: o avião some, e ~3s depois reaparece no ponto inicial com metade do tanque.
- Voar normal (sem boost) nunca zera o tanque.

- [ ] **Step 3: Anotar e ajustar**

As perguntas que só o playtest responde, e o parâmetro de cada uma em `FFuelParams` (`Source/FutebolAviao/Flight/FuelSystem.h`):

| Se no playtest... | Ajustar |
|---|---|
| o boost acaba rápido demais | `BoostDrainPerSec` (25) pra baixo, ou `TankCapacity` (100) pra cima |
| o boost é grátis / dá pra segurar sempre | `BoostDrainPerSec` pra cima, ou `PassiveRegenPerSec` (8) pra baixo |
| a espera pra reabastecer é chata | `PassiveRegenPerSec` pra cima, ou `RegenDelayAfterBoostSec` (1.5) pra baixo |
| explodir não dói | `RespawnSeconds` (3) pra cima, ou `RespawnFuelFraction` (0.5) pra baixo |
| explodir pune demais | o inverso |
| o boost não parece mais rápido | `BoostMaxSpeed` (9000) / `BoostAcceleration` (5000) em `FFlightPhysicsParams` pra cima |

Depois de cada ajuste, rodar os dois conjuntos de testes standalone — os testes fixam os próprios params, então mudar defaults não deve quebrar nada. Se quebrar, o teste estava dependendo de um default por acidente: tornar a dependência explícita no teste, como foi feito na Fase 1 com o `MinSpeed`.

**Lição da Fase 1 que vale aqui:** um default "fisicamente correto" pode ser péssimo de jogar. O `MinSpeed = 1400` ("avião nunca para no ar") foi reprovado no playtest porque o avião saía voando sozinho no Play. Quem decide é o playtest, não o argumento.

- [ ] **Step 4: Commit**

```bash
git add Source/FutebolAviao/Flight/FuelSystem.h Source/FutebolAviao/Flight/FlightPhysics.h
git commit -m "tune: adjust fuel and boost feel after playtest"
```

---

## Ao final da Fase 2

O avião tem um recurso pra gerenciar: correr atrás de algo custa combustível, e ficar sem custa alguns segundos fora da jogada. Isso é o que dá peso à disputa quando a bola entrar, na Fase 3.

Duas coisas ficaram deliberadamente de fora e devem entrar depois:
- **Pickups de combustível** no campo, em posições randomizadas e escalando com o número de jogadores — a spec põe isso junto do 2v2 local (Fase 4), porque a quantidade só faz sentido com mais de um jogador.
- **Replicação e autoridade do servidor** sobre o combustível — Fase 5. Manter `FFuelSystem` puro é o que vai tornar essa migração barata: a regra já está separada do ator.
