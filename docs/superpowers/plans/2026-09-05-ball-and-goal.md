# Fase 3 — Bola e Gol — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Uma bola física numa arena fechada, que os aviões conseguem acertar e empurrar, e dois gols que contam ponto — o primeiro momento em que o jogo é um jogo.

**Architecture:** Mesmo padrão das Fases 1 e 2, que já se pagou duas vezes: a regra vira classe C++ pura sem nenhum header da Unreal, testada por executável standalone; o ator é um adapter fino. `FBallPhysics` cuida de gravidade, arrasto e quique nas paredes; `FMatchRules` cuida de detectar gol e contar placar. `ABallActor` e `AFutebolAviaoGameModeBase` são os adapters.

**Tech Stack:** Unreal Engine 5.8, C++17, sem Blueprints.

## Decisão de arquitetura que contraria a spec — ler antes de começar

A spec diz "usar o Chaos Physics de forma econômica". **Este plano não usa Chaos para a bola.** A bola é simulada por uma classe pura própria, pelos mesmos motivos que já valeram para o voo e o combustível:

- A Fase 5 exige servidor autoritativo **com client-side prediction**. Prediction precisa de re-simulação determinística a partir de um estado. Chaos não dá isso de graça; uma classe pura com `Update(State&, dt)` dá.
- As 22 regras que já existem são testadas sem abrir a engine. Uma bola em Chaos só se testa em Automation test, que é ordens de magnitude mais lento e mais frágil.
- A arena do protótipo é geometricamente trivial (uma caixa). Quique numa caixa é aritmética de duas linhas; não justifica um motor de física.

Se a bola precisar um dia de rotação, spin ou colisão com malha complexa, essa decisão se revisita. Enquanto for uma esfera numa caixa, pura ganha.

## Global Constraints

- Módulo Unreal: `FutebolAviao`. Engine `5.8`, Win64, C++17. Sem Blueprints.
- **`PureMath.h`, `FlightPhysics.{h,cpp}`, `FuelSystem.{h,cpp}` e os novos `BallPhysics.{h,cpp}` e `MatchRules.{h,cpp}` não podem incluir NENHUM header da Unreal.** As suítes em `Tools/` compilam só `<Nome>Tests.cpp` mais `<Nome>.cpp`, então qualquer header compartilhado tem que ser header-only.
- Adapters (`APlanePawn`, `ABallActor`, `AFutebolAviaoGameModeBase`) são o único lugar com tipo da engine.
- **Não** introduzir `Replicated`/RPC — isso é Fase 5. Manter as regras puras é o que torna aquela migração barata.
- Fora do escopo desta fase: times, 2v2, pickups de combustível, tempo de partida, HUD com arte, rede.
- Estilo: tabs, chaves Allman, comentários em português **sem acentos** (o código é estritamente ASCII).
- Fechar `UnrealEditor.exe` e `LiveCodingConsole.exe` antes de compilar por linha de comando.

**Rodar os testes** (harness commitado, sai com código != 0 se algo falhar):

```powershell
powershell -ExecutionPolicy Bypass -File Tools\run-tests.ps1
```

**Compilar o editor:**

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FutebolAviaoEditor Win64 Development -Project="C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -WaitMutex
```

Sucesso é `Result: Succeeded`. O aviso `'vswhere.exe' is not recognized` é ruído esperado.

---

### Task 1: Vetor compartilhado em PureMath.h

**Files:**
- Modify: `Source/FutebolAviao/Flight/PureMath.h`
- Modify: `Source/FutebolAviao/Flight/FlightPhysics.h`, `FlightPhysics.cpp`
- Modify: `Source/FutebolAviao/Flight/PlanePawn.cpp`
- Modify: `Tools/FlightPhysicsTests/FlightPhysicsTests.cpp`

**Interfaces:**
- Produces: `PureMath::FPureVector` (campos `X`, `Y`, `Z`, todos `float`, todos default `0.f`) e as funções livres `Length`, `Normalized`, `Add`, `Subtract`, `Scale`, `Dot` no mesmo namespace. As Tasks 2 a 4 usam exatamente esses nomes.
- Consumes: nada.

Hoje `FFlightVector` mora em `FlightPhysics.h`. A bola precisa do mesmo tipo, e fazer `BallPhysics.h` incluir `FlightPhysics.h` acoplaria dois sistemas que não têm relação. O tipo sobe para `PureMath.h`, que já é a casa do que é compartilhado e puro.

Esta task **não muda comportamento nenhum**. Os 22 testes existentes devem passar sem que nenhum valor esperado mude.

- [ ] **Step 1: Mover o tipo e adicionar as operações**

Em `Source/FutebolAviao/Flight/PureMath.h`, dentro de `namespace PureMath`, depois de `WrapDegrees`:

```cpp
	// Vetor proprio: as classes puras nao podem usar FVector. Os adapters
	// convertem na fronteira.
	struct FPureVector
	{
		float X = 0.f;
		float Y = 0.f;
		float Z = 0.f;
	};

	inline float Dot(const FPureVector& A, const FPureVector& B)
	{
		return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
	}

	inline float Length(const FPureVector& V)
	{
		return std::sqrt(Dot(V, V));
	}

	inline FPureVector Add(const FPureVector& A, const FPureVector& B)
	{
		FPureVector Result;
		Result.X = A.X + B.X;
		Result.Y = A.Y + B.Y;
		Result.Z = A.Z + B.Z;
		return Result;
	}

	inline FPureVector Subtract(const FPureVector& A, const FPureVector& B)
	{
		FPureVector Result;
		Result.X = A.X - B.X;
		Result.Y = A.Y - B.Y;
		Result.Z = A.Z - B.Z;
		return Result;
	}

	inline FPureVector Scale(const FPureVector& V, float Factor)
	{
		FPureVector Result;
		Result.X = V.X * Factor;
		Result.Y = V.Y * Factor;
		Result.Z = V.Z * Factor;
		return Result;
	}

	// Devolve o vetor unitario. Vetor de comprimento desprezivel devolve zero --
	// quem chama decide o fallback, porque a direcao certa depende do contexto.
	inline FPureVector Normalized(const FPureVector& V)
	{
		const float Len = Length(V);
		if (Len < 0.0001f)
		{
			return FPureVector();
		}
		return Scale(V, 1.f / Len);
	}
```

E no topo do arquivo, logo depois do `#pragma once`:

```cpp
#include <cmath>
```

- [ ] **Step 2: Apagar o tipo antigo e reapontar todo mundo**

Em `Source/FutebolAviao/Flight/FlightPhysics.h`: apagar o `struct FFlightVector` inteiro (com o comentário acima dele), adicionar `#include "PureMath.h"` logo depois do `#pragma once`, e trocar o campo do estado por:

```cpp
	PureMath::FPureVector Velocity;
```

Depois, substituir toda ocorrência restante de `FFlightVector` por `PureMath::FPureVector` em `FlightPhysics.cpp`, `PlanePawn.cpp` e `Tools/FlightPhysicsTests/FlightPhysicsTests.cpp`. Onde `FlightPhysics.cpp` já tiver helpers locais de vetor (comprimento, normalização, mistura), preferir os de `PureMath` e apagar os locais — mas **sem alterar a aritmética**: se um helper local fizer algo diferente do de `PureMath`, manter o local e dizer isso no relatório em vez de mudar o resultado.

- [ ] **Step 3: Rodar os testes**

```powershell
powershell -ExecutionPolicy Bypass -File Tools\run-tests.ps1
```

Esperado: as duas suítes passam, 22 testes, exit code 0. **Se qualquer valor esperado precisar mudar, parar e reportar** — significa que a aritmética mudou, o que esta task não pode fazer.

- [ ] **Step 4: Compilar o editor**

Esperado: `Result: Succeeded`.

- [ ] **Step 5: Commit**

```bash
git add Source/FutebolAviao/Flight/PureMath.h Source/FutebolAviao/Flight/FlightPhysics.h Source/FutebolAviao/Flight/FlightPhysics.cpp Source/FutebolAviao/Flight/PlanePawn.cpp Tools/FlightPhysicsTests/FlightPhysicsTests.cpp
git commit -m "refactor: move the pure vector type into PureMath.h"
```

---

### Task 2: Física da bola (pura)

**Files:**
- Create: `Source/FutebolAviao/Flight/BallPhysics.h`, `BallPhysics.cpp`
- Test: `Tools/BallPhysicsTests/BallPhysicsTests.cpp`

**Interfaces:**
- Consumes: `PureMath::FPureVector` e helpers da Task 1.
- Produces: `struct FBallPhysicsParams`, `struct FBallState`, `class FBallPhysics` com `Update(FBallState&, float)`. As Tasks 3 e 4 estendem essa mesma classe e leem esse mesmo estado.

A arena é uma caixa. `ArenaHalfX`/`ArenaHalfY` são as metades das dimensões; o chão é `Z = 0` e o teto `ArenaCeilingZ`. A bola quica em todas as seis faces.

- [ ] **Step 1: Escrever o primeiro teste (vai falhar — a classe não existe)**

Criar `Tools/BallPhysicsTests/BallPhysicsTests.cpp`:

```cpp
#include "../../Source/FutebolAviao/Flight/BallPhysics.h"
#include <cassert>
#include <cstdio>
#include <cmath>

static bool NearlyEqual(float A, float B, float Tolerance = 0.01f)
{
	return std::fabs(A - B) <= Tolerance;
}

static void Test_GravityPullsTheBallDown()
{
	FBallPhysicsParams Params;
	Params.Gravity = 980.f;
	Params.Drag = 0.f;
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.Z = 3000.f;

	Ball.Update(State, 1.f);

	assert(NearlyEqual(State.Velocity.Z, -980.f));
	printf("Test_GravityPullsTheBallDown passed\n");
}

int main()
{
	Test_GravityPullsTheBallDown();
	printf("All tests passed\n");
	return 0;
}
```

- [ ] **Step 2: Rodar e confirmar que falha**

```powershell
powershell -ExecutionPolicy Bypass -File Tools\run-tests.ps1
```

Esperado: erro de compilação `Cannot open include file: '../../Source/FutebolAviao/Flight/BallPhysics.h'`. O harness descobre a suíte nova sozinho; se não descobrir, ajustar o harness antes de seguir.

- [ ] **Step 3: Criar o header**

Criar `Source/FutebolAviao/Flight/BallPhysics.h`:

```cpp
#pragma once

#include "PureMath.h"

struct FBallPhysicsParams
{
	float Gravity = 980.f;          // cm/s^2, puxando pra baixo
	float Drag = 0.35f;             // fracao da velocidade perdida por segundo
	float Restitution = 0.75f;      // quanto da velocidade sobra depois de quicar
	float Radius = 150.f;
	float MaxSpeed = 12000.f;

	// Arena: caixa fechada. X e Y sao metades; o chao e Z=0.
	float ArenaHalfX = 10000.f;
	float ArenaHalfY = 6000.f;
	float ArenaCeilingZ = 5000.f;
};

struct FBallState
{
	PureMath::FPureVector Position;
	PureMath::FPureVector Velocity;
};

class FBallPhysics
{
public:
	FBallPhysics() : Params() {}
	explicit FBallPhysics(const FBallPhysicsParams& InParams) : Params(InParams) {}

	void Update(FBallState& State, float DeltaSeconds) const;

	const FBallPhysicsParams& GetParams() const { return Params; }

private:
	FBallPhysicsParams Params;
};
```

Criar `Source/FutebolAviao/Flight/BallPhysics.cpp` com o mínimo pro teste passar:

```cpp
#include "BallPhysics.h"

void FBallPhysics::Update(FBallState& State, float DeltaSeconds) const
{
	State.Velocity.Z -= Params.Gravity * DeltaSeconds;
	State.Position = PureMath::Add(State.Position, PureMath::Scale(State.Velocity, DeltaSeconds));
}
```

- [ ] **Step 4: Rodar e confirmar que passa**

- [ ] **Step 5: Commit**

```bash
git add Source/FutebolAviao/Flight/BallPhysics.h Source/FutebolAviao/Flight/BallPhysics.cpp Tools/BallPhysicsTests/BallPhysicsTests.cpp
git commit -m "feat: add pure ball physics with gravity"
```

- [ ] **Step 6: Teste do quique no chão (vai falhar)**

Inserir antes de `int main()`:

```cpp
static void Test_BallBouncesOffTheFloorLosingEnergy()
{
	FBallPhysicsParams Params;
	Params.Gravity = 0.f;
	Params.Drag = 0.f;
	Params.Restitution = 0.75f;
	Params.Radius = 150.f;
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.Z = 200.f;
	State.Velocity.Z = -100.f;

	Ball.Update(State, 1.f);   // desceria pra 100, abaixo do raio: quica

	assert(NearlyEqual(State.Position.Z, 150.f));   // pousada em cima do chao
	assert(NearlyEqual(State.Velocity.Z, 75.f));    // 100 * 0.75, agora subindo
	printf("Test_BallBouncesOffTheFloorLosingEnergy passed\n");
}
```

E em `main()`, depois de `Test_GravityPullsTheBallDown();`:

```cpp
	Test_BallBouncesOffTheFloorLosingEnergy();
```

- [ ] **Step 7: Rodar e confirmar que falha**

Esperado: `Assertion failed: NearlyEqual(State.Position.Z, 150.f)` — sem quique, a bola atravessa o chão e para em 100.

- [ ] **Step 8: Implementar o quique nas seis faces**

Substituir o corpo de `FBallPhysics::Update` em `BallPhysics.cpp`:

```cpp
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
	State.Position = PureMath::Add(State.Position, PureMath::Scale(State.Velocity, DeltaSeconds));

	const float R = Params.Radius;
	BounceAxis(State.Position.X, State.Velocity.X, -Params.ArenaHalfX + R, Params.ArenaHalfX - R, Params.Restitution);
	BounceAxis(State.Position.Y, State.Velocity.Y, -Params.ArenaHalfY + R, Params.ArenaHalfY - R, Params.Restitution);
	BounceAxis(State.Position.Z, State.Velocity.Z, R, Params.ArenaCeilingZ - R, Params.Restitution);
}
```

- [ ] **Step 9: Rodar e confirmar que os dois passam**

- [ ] **Step 10: Teste do arrasto (vai falhar)**

Inserir antes de `int main()`:

```cpp
static void Test_DragSlowsTheBallDown()
{
	FBallPhysicsParams Params;
	Params.Gravity = 0.f;
	Params.Drag = 0.5f;
	Params.ArenaHalfX = 100000.f;
	Params.ArenaHalfY = 100000.f;
	Params.ArenaCeilingZ = 100000.f;
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.Z = 5000.f;
	State.Velocity.X = 1000.f;

	Ball.Update(State, 1.f);

	assert(State.Velocity.X < 1000.f);
	assert(State.Velocity.X > 0.f);   // desacelera, nao inverte nem zera
	printf("Test_DragSlowsTheBallDown passed\n");
}
```

E registrar em `main()` depois de `Test_BallBouncesOffTheFloorLosingEnergy();`:

```cpp
	Test_DragSlowsTheBallDown();
```

- [ ] **Step 11: Rodar e confirmar que falha**

Esperado: `Assertion failed: State.Velocity.X < 1000.f` — sem arrasto, a velocidade fica exatamente 1000.

- [ ] **Step 12: Implementar arrasto e teto de velocidade**

Em `BallPhysics.cpp`, dentro de `Update`, **antes** da linha que integra a posição:

```cpp
	// Arrasto exponencial: fracao da velocidade perdida por segundo, estavel em
	// qualquer DeltaSeconds (ao contrario de subtrair uma constante).
	const float DragFactor = PureMath::ClampValue(1.f - Params.Drag * DeltaSeconds, 0.f, 1.f);
	State.Velocity = PureMath::Scale(State.Velocity, DragFactor);

	const float Speed = PureMath::Length(State.Velocity);
	if (Speed > Params.MaxSpeed)
	{
		State.Velocity = PureMath::Scale(PureMath::Normalized(State.Velocity), Params.MaxSpeed);
	}
```

- [ ] **Step 13: Rodar e confirmar que os três passam**

- [ ] **Step 14: Commit**

```bash
git add Source/FutebolAviao/Flight/BallPhysics.cpp Tools/BallPhysicsTests/BallPhysicsTests.cpp
git commit -m "feat: add arena bounce, drag and speed cap to the ball"
```

---

### Task 3: Avião acerta a bola

**Files:**
- Modify: `Source/FutebolAviao/Flight/BallPhysics.h`, `BallPhysics.cpp`
- Modify: `Tools/BallPhysicsTests/BallPhysicsTests.cpp`

**Interfaces:**
- Consumes: `FBallPhysics` da Task 2.
- Produces: `bool FBallPhysics::IsOverlapping(const FBallState&, const PureMath::FPureVector& PlanePosition, float PlaneRadius) const` e `void FBallPhysics::ApplyHit(FBallState&, const PureMath::FPureVector& PlanePosition, const PureMath::FPureVector& PlaneVelocity, float PlaneRadius) const`. A Task 5 chama os dois, nessa ordem, uma vez por frame.

- [ ] **Step 1: Teste do impulso (vai falhar)**

Inserir antes de `int main()` em `Tools/BallPhysicsTests/BallPhysicsTests.cpp`:

```cpp
static void Test_PlaneHitPushesTheBallAway()
{
	FBallPhysicsParams Params;
	Params.Radius = 150.f;
	Params.HitTransfer = 1.6f;
	Params.MinKick = 500.f;
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.X = 400.f;   // bola a frente do aviao, no eixo X

	PureMath::FPureVector PlanePosition;   // aviao na origem
	PureMath::FPureVector PlaneVelocity;
	PlaneVelocity.X = 3000.f;              // voando em cima dela

	Ball.ApplyHit(State, PlanePosition, PlaneVelocity, /*PlaneRadius*/ 300.f);

	assert(State.Velocity.X > 3000.f);                  // levou impulso pra frente
	assert(NearlyEqual(State.Velocity.Y, 0.f));
	assert(NearlyEqual(State.Position.X, 450.f));       // empurrada pra fora da sobreposicao
	printf("Test_PlaneHitPushesTheBallAway passed\n");
}
```

E em `main()`, depois de `Test_DragSlowsTheBallDown();`:

```cpp
	Test_PlaneHitPushesTheBallAway();
```

- [ ] **Step 2: Rodar e confirmar que falha**

Esperado: erros de compilação — `'HitTransfer': is not a member of 'FBallPhysicsParams'` e `'ApplyHit': is not a member of 'FBallPhysics'`.

- [ ] **Step 3: Declarar os parâmetros e os métodos**

Em `BallPhysics.h`, dentro de `FBallPhysicsParams`, depois de `MaxSpeed`:

```cpp
	float HitTransfer = 1.6f;   // quanto da velocidade de aproximacao do aviao vira impulso
	float MinKick = 500.f;      // toque de raspao ainda mexe com a bola
```

E em `FBallPhysics`, depois de `Update`:

```cpp
	// True quando as esferas de aviao e bola se tocam.
	bool IsOverlapping(const FBallState& State, const PureMath::FPureVector& PlanePosition, float PlaneRadius) const;

	// Impulso do aviao na bola, mais o empurrao que desfaz a sobreposicao -- sem
	// ele o aviao "carrega" a bola, reacertando todo frame.
	void ApplyHit(FBallState& State, const PureMath::FPureVector& PlanePosition, const PureMath::FPureVector& PlaneVelocity, float PlaneRadius) const;
```

- [ ] **Step 4: Implementar**

No fim de `BallPhysics.cpp`:

```cpp
bool FBallPhysics::IsOverlapping(const FBallState& State, const PureMath::FPureVector& PlanePosition, float PlaneRadius) const
{
	const float Distance = PureMath::Length(PureMath::Subtract(State.Position, PlanePosition));
	return Distance < (Params.Radius + PlaneRadius);
}

void FBallPhysics::ApplyHit(FBallState& State, const PureMath::FPureVector& PlanePosition, const PureMath::FPureVector& PlaneVelocity, float PlaneRadius) const
{
	PureMath::FPureVector Normal = PureMath::Normalized(PureMath::Subtract(State.Position, PlanePosition));
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

	const float Approach = PureMath::Dot(PlaneVelocity, Normal);
	const float Impulse = (Approach > 0.f ? Approach * Params.HitTransfer : 0.f) + Params.MinKick;

	State.Velocity = PureMath::Add(State.Velocity, PureMath::Scale(Normal, Impulse));

	const float Speed = PureMath::Length(State.Velocity);
	if (Speed > Params.MaxSpeed)
	{
		State.Velocity = PureMath::Scale(PureMath::Normalized(State.Velocity), Params.MaxSpeed);
	}

	State.Position = PureMath::Add(PlanePosition, PureMath::Scale(Normal, Params.Radius + PlaneRadius));
}
```

- [ ] **Step 5: Rodar e confirmar que passa**

- [ ] **Step 6: Teste do toque de raspão (vai falhar se o MinKick sumir)**

Inserir antes de `int main()`:

```cpp
static void Test_StationaryPlaneStillNudgesTheBall()
{
	FBallPhysicsParams Params;
	Params.Radius = 150.f;
	Params.MinKick = 500.f;
	FBallPhysics Ball(Params);
	FBallState State;
	State.Position.X = 400.f;

	PureMath::FPureVector PlanePosition;
	PureMath::FPureVector PlaneVelocity;   // aviao parado

	Ball.ApplyHit(State, PlanePosition, PlaneVelocity, 300.f);

	assert(NearlyEqual(State.Velocity.X, 500.f));   // so o MinKick
	printf("Test_StationaryPlaneStillNudgesTheBall passed\n");
}
```

E registrar em `main()` depois de `Test_PlaneHitPushesTheBallAway();`:

```cpp
	Test_StationaryPlaneStillNudgesTheBall();
```

- [ ] **Step 7: Rodar e confirmar que passa, e provar que o teste tem dentes**

Este teste passa de primeira. Para provar que ele não é decorativo: mudar temporariamente `+ Params.MinKick` para `+ 0.f` em `ApplyHit`, rodar, confirmar que **falha**, restaurar, confirmar verde. Reportar o texto exato da assertion que falhou.

- [ ] **Step 8: Commit**

```bash
git add Source/FutebolAviao/Flight/BallPhysics.h Source/FutebolAviao/Flight/BallPhysics.cpp Tools/BallPhysicsTests/BallPhysicsTests.cpp
git commit -m "feat: let planes hit the ball with an impulse and push-out"
```

---

### Task 4: Gol e placar (puro)

**Files:**
- Create: `Source/FutebolAviao/Flight/MatchRules.h`, `MatchRules.cpp`
- Test: `Tools/MatchRulesTests/MatchRulesTests.cpp`

**Interfaces:**
- Consumes: `PureMath::FPureVector` da Task 1; `FBallState` da Task 2.
- Produces: `struct FMatchParams`, `struct FMatchState`, `class FMatchRules` com `EGoalSide CheckGoal(const PureMath::FPureVector& BallPosition) const` e `void RegisterGoal(FMatchState&, EGoalSide) const`. A Task 5 chama os dois.

Os gols ficam nas duas faces `X = ±ArenaHalfX`, centrados em Y. O lado Oeste é `X` negativo; marcar nele é ponto do time Leste, e vice-versa.

- [ ] **Step 1: Escrever o primeiro teste (vai falhar — a classe não existe)**

Criar `Tools/MatchRulesTests/MatchRulesTests.cpp`:

```cpp
#include "../../Source/FutebolAviao/Flight/MatchRules.h"
#include <cassert>
#include <cstdio>

static void Test_BallInsideWestGoalCountsForEast()
{
	FMatchParams Params;
	Params.ArenaHalfX = 10000.f;
	Params.GoalHalfWidthY = 1500.f;
	Params.GoalHeightZ = 2000.f;
	FMatchRules Rules(Params);

	PureMath::FPureVector Ball;
	Ball.X = -10050.f;   // passou da linha oeste
	Ball.Y = 0.f;
	Ball.Z = 800.f;

	assert(Rules.CheckGoal(Ball) == EGoalSide::West);
	printf("Test_BallInsideWestGoalCountsForEast passed\n");
}

int main()
{
	Test_BallInsideWestGoalCountsForEast();
	printf("All tests passed\n");
	return 0;
}
```

- [ ] **Step 2: Rodar e confirmar que falha**

Esperado: `Cannot open include file: '../../Source/FutebolAviao/Flight/MatchRules.h'`.

- [ ] **Step 3: Criar o header e a implementação**

Criar `Source/FutebolAviao/Flight/MatchRules.h`:

```cpp
#pragma once

#include "PureMath.h"

enum class EGoalSide
{
	None,
	West,
	East
};

struct FMatchParams
{
	float ArenaHalfX = 10000.f;
	float GoalHalfWidthY = 1500.f;
	float GoalHeightZ = 2000.f;
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
```

Criar `Source/FutebolAviao/Flight/MatchRules.cpp`:

```cpp
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
```

- [ ] **Step 4: Rodar e confirmar que passa**

- [ ] **Step 5: Teste da bola que sai por fora do gol (vai falhar)**

Inserir antes de `int main()`:

```cpp
static void Test_BallPastTheLineButOutsideTheMouthIsNotAGoal()
{
	FMatchParams Params;
	Params.ArenaHalfX = 10000.f;
	Params.GoalHalfWidthY = 1500.f;
	Params.GoalHeightZ = 2000.f;
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
```

E registrar em `main()`.

- [ ] **Step 6: Rodar**

Este passa direto, porque o Step 3 já implementou a boca do gol. Para provar que tem dentes: remover temporariamente a checagem `bInsideMouth` de `CheckGoal`, rodar, confirmar que **falha**, restaurar, confirmar verde. Reportar o texto da assertion.

- [ ] **Step 7: Teste do placar (vai falhar)**

Inserir antes de `int main()`:

```cpp
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
```

E registrar em `main()`.

- [ ] **Step 8: Rodar e confirmar que as três passam**

- [ ] **Step 9: Commit**

```bash
git add Source/FutebolAviao/Flight/MatchRules.h Source/FutebolAviao/Flight/MatchRules.cpp Tools/MatchRulesTests/MatchRulesTests.cpp
git commit -m "feat: add pure goal detection and scoring"
```

---

### Task 5: Ligar bola, gol e placar na engine

**Files:**
- Create: `Source/FutebolAviao/Flight/BallActor.h`, `BallActor.cpp`
- Modify: `Source/FutebolAviao/FutebolAviaoGameModeBase.h`, `.cpp`
- Modify: `Source/FutebolAviao/Flight/PlanePawn.h`, `PlanePawn.cpp`
- Modify: `Tools/LevelBuilder/BuildTestFlightMap.py`
- Modify: `Content/Maps/TestFlightMap.umap` (regenerado pelo script)

**Interfaces:**
- Consumes: tudo das Tasks 2 a 4.
- Produces: entrega final de código da fase.

Esta é a única task com tipo da engine. `ABallActor` tica a física pura e move a esfera; o GameMode guarda `FMatchState` e checa gol; `APlanePawn` expõe posição e velocidade para o ator da bola conseguir testar a colisão.

- [ ] **Step 1: Expor a posição e a velocidade do avião**

Em `Source/FutebolAviao/Flight/PlanePawn.h`, na seção `public:`:

```cpp
	// A bola precisa saber onde o aviao esta e quao rapido vai, pra resolver o
	// impacto. Raio esferico aproximado do aviao, generoso de proposito: acertar
	// a bola tem que ser divertido, nao preciso.
	UFUNCTION(BlueprintCallable, Category = "Plane")
	float GetCollisionRadius() const { return 300.f; }

	PureMath::FPureVector GetPureVelocity() const { return FlightState.Velocity; }
```

- [ ] **Step 2: Criar o ator da bola**

Criar `Source/FutebolAviao/Flight/BallActor.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BallPhysics.h"
#include "BallActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class FUTEBOLAVIAO_API ABallActor : public AActor
{
	GENERATED_BODY()

public:
	ABallActor();

	// Volta pro centro da arena, parada. Chamado no inicio e depois de cada gol.
	void ResetToCenter();

	const FBallState& GetBallState() const { return BallState; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Ball")
	UStaticMeshComponent* MeshComponent;

	FBallPhysics BallPhysics;
	FBallState BallState;
};
```

Criar `Source/FutebolAviao/Flight/BallActor.cpp`:

```cpp
#include "BallActor.h"
#include "PlanePawn.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"

namespace
{
	PureMath::FPureVector ToPure(const FVector& V)
	{
		PureMath::FPureVector Result;
		Result.X = static_cast<float>(V.X);
		Result.Y = static_cast<float>(V.Y);
		Result.Z = static_cast<float>(V.Z);
		return Result;
	}

	FVector ToUnreal(const PureMath::FPureVector& V)
	{
		return FVector(V.X, V.Y, V.Z);
	}
}

ABallActor::ABallActor()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(SphereAsset.Object);
		// A esfera do Engine tem 50 de raio; escalar pro raio dos params.
		const float Scale = BallPhysics.GetParams().Radius / 50.f;
		MeshComponent->SetRelativeScale3D(FVector(Scale, Scale, Scale));
	}
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABallActor::BeginPlay()
{
	Super::BeginPlay();
	ResetToCenter();
}

void ABallActor::ResetToCenter()
{
	BallState = FBallState();
	BallState.Position.Z = 1500.f;
	SetActorLocation(ToUnreal(BallState.Position));
}

void ABallActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Um aviao por vez basta nesta fase; a Fase 4 traz quatro e o laco continua
	// valendo.
	for (TActorIterator<APlanePawn> It(GetWorld()); It; ++It)
	{
		APlanePawn* Plane = *It;
		if (!Plane || Plane->IsHidden())
		{
			continue;   // aviao explodido nao acerta bola
		}

		const PureMath::FPureVector PlanePosition = ToPure(Plane->GetActorLocation());
		if (BallPhysics.IsOverlapping(BallState, PlanePosition, Plane->GetCollisionRadius()))
		{
			BallPhysics.ApplyHit(BallState, PlanePosition, Plane->GetPureVelocity(), Plane->GetCollisionRadius());
		}
	}

	BallPhysics.Update(BallState, DeltaSeconds);
	SetActorLocation(ToUnreal(BallState.Position));
}
```

- [ ] **Step 3: GameMode guarda o placar e checa gol**

Em `Source/FutebolAviao/FutebolAviaoGameModeBase.h`, dentro da classe:

```cpp
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	FMatchRules MatchRules;
	FMatchState MatchState;

	UPROPERTY()
	class ABallActor* Ball = nullptr;
```

E no topo, junto dos includes:

```cpp
#include "Flight/MatchRules.h"
```

Em `FutebolAviaoGameModeBase.cpp`, adicionar os includes e os dois métodos:

```cpp
#include "Flight/BallActor.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
```

```cpp
void AFutebolAviaoGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	PrimaryActorTick.bCanEverTick = true;

	for (TActorIterator<ABallActor> It(GetWorld()); It; ++It)
	{
		Ball = *It;
		break;
	}

	if (!Ball)
	{
		Ball = GetWorld()->SpawnActor<ABallActor>(ABallActor::StaticClass(), FVector(0.f, 0.f, 1500.f), FRotator::ZeroRotator);
	}
}

void AFutebolAviaoGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Ball)
	{
		return;
	}

	const EGoalSide Side = MatchRules.CheckGoal(Ball->GetBallState().Position);
	if (Side != EGoalSide::None)
	{
		MatchRules.RegisterGoal(MatchState, Side);
		Ball->ResetToCenter();
	}

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 0.05f, FColor::Cyan,
			FString::Printf(TEXT("Oeste %d  x  %d Leste"), MatchState.WestScore, MatchState.EastScore));
	}
#endif
}
```

No construtor de `AFutebolAviaoGameModeBase`, adicionar:

```cpp
	PrimaryActorTick.bCanEverTick = true;
```

- [ ] **Step 4: Arena com paredes e gols visíveis**

Em `Tools/LevelBuilder/BuildTestFlightMap.py`, depois dos `PlayerStart`, adicionar paredes e as duas bocas de gol. As dimensões têm que bater com os defaults de `FBallPhysicsParams` e `FMatchParams` (`ArenaHalfX = 10000`, `ArenaHalfY = 6000`, `ArenaCeilingZ = 5000`, `GoalHalfWidthY = 1500`, `GoalHeightZ = 2000`):

```python
ARENA_HALF_X = 10000.0
ARENA_HALF_Y = 6000.0
GOAL_HALF_WIDTH_Y = 1500.0
GOAL_HEIGHT_Z = 2000.0


def spawn_box(actors, label, location, scale):
    """Cubo estatico usado como parede. O cubo do Engine tem 100 de lado."""
    box = spawn(actors, unreal.StaticMeshActor, location)
    box.set_actor_label(label)
    box.set_actor_scale3d(scale)
    box.static_mesh_component.set_static_mesh(unreal.EditorAssetLibrary.load_asset(CUBE_MESH))
    return box
```

E dentro de `build()`, antes do `save_current_level()`:

```python
    # Paredes laterais (eixo Y): compridas em X, finas em Y.
    for side, sign in (("Norte", 1.0), ("Sul", -1.0)):
        spawn_box(actors, "Parede_" + side,
                  unreal.Vector(0.0, sign * ARENA_HALF_Y, 2500.0),
                  unreal.Vector(ARENA_HALF_X * 2 / 100.0, 1.0, 50.0))

    # Fundos (eixo X), com a boca do gol aberta no meio: dois blocos por lado.
    for side, sign in (("Oeste", -1.0), ("Leste", 1.0)):
        # Blocos laterais, deixando GOAL_HALF_WIDTH_Y livre no centro.
        side_width = (ARENA_HALF_Y - GOAL_HALF_WIDTH_Y)
        for edge, edge_sign in (("A", 1.0), ("B", -1.0)):
            center_y = edge_sign * (GOAL_HALF_WIDTH_Y + side_width / 2.0)
            spawn_box(actors, "Fundo_%s_%s" % (side, edge),
                      unreal.Vector(sign * ARENA_HALF_X, center_y, 2500.0),
                      unreal.Vector(1.0, side_width / 100.0, 50.0))
        # Travessao: fecha por cima da boca.
        spawn_box(actors, "Travessao_" + side,
                  unreal.Vector(sign * ARENA_HALF_X, 0.0, GOAL_HEIGHT_Z + 1500.0),
                  unreal.Vector(1.0, GOAL_HALF_WIDTH_Y * 2 / 100.0, 30.0))
```

Regenerar o mapa:

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -run=pythonscript -script="C:\Users\cauad\Desktop\dev\jogao\Tools\LevelBuilder\BuildTestFlightMap.py" -unattended -nosplash -nop4
```

Confirmar `Python script executed successfully` e que `Content/Maps/TestFlightMap.umap` foi reescrito.

- [ ] **Step 5: Compilar o editor**

Esperado: `Result: Succeeded`.

- [ ] **Step 6: Rodar todas as suítes**

```powershell
powershell -ExecutionPolicy Bypass -File Tools\run-tests.ps1
```

Esperado: quatro suítes (FuelSystem, FlightPhysics, BallPhysics, MatchRules), todas verdes, exit code 0. Esta task não adiciona regra nova — rodar as suítes é a verificação de que nada regrediu.

- [ ] **Step 7: Commit**

```bash
git add Source/FutebolAviao/Flight/BallActor.h Source/FutebolAviao/Flight/BallActor.cpp Source/FutebolAviao/Flight/PlanePawn.h Source/FutebolAviao/FutebolAviaoGameModeBase.h Source/FutebolAviao/FutebolAviaoGameModeBase.cpp Tools/LevelBuilder/BuildTestFlightMap.py Content/Maps/TestFlightMap.umap
git commit -m "feat: wire ball, arena walls and scoring into the level"
```

---

### Task 6: Playtest

**Files:** nenhum, a menos que o playtest peça ajuste.

- [ ] **Step 1: Abrir e jogar**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject"
```

Alt+P. O texto ciano mostra o placar; o amarelo, combustível e velocidade.

- [ ] **Step 2: Conferir**

- A bola cai, quica no chão e perde energia a cada quique.
- Voar em cima dela empurra na direção do voo; passar rente dá um toque leve.
- A bola quica nas paredes e não escapa da arena.
- Entrar na boca do gol conta ponto e a bola volta pro centro.
- Bola por cima do travessão ou por fora do poste **não** conta.
- O avião não fica "carregando" a bola grudada.

- [ ] **Step 3: Anotar e ajustar**

| Se no playtest... | Ajustar em |
|---|---|
| a bola parece pesada/morta | `Restitution` (0.75) e `Drag` (0.35) em `FBallPhysicsParams` |
| acertar a bola é difícil demais | `GetCollisionRadius` (300) em `PlanePawn.h` |
| o toque manda a bola longe demais | `HitTransfer` (1.6) e `MinKick` (500) |
| a bola cai rápido demais | `Gravity` (980) |
| fazer gol é fácil/difícil demais | `GoalHalfWidthY` (1500) e `GoalHeightZ` (2000) em `FMatchParams` — lembrar de regenerar o mapa depois, senão a boca visível e a boca lógica divergem |

**Armadilha registrada:** as dimensões da arena existem em dois lugares — nos params C++ e no script Python do nível. Se divergirem, a bola quica no nada ou atravessa parede visível. Mudou um, regenerar o outro.

- [ ] **Step 4: Commit**

```bash
git add Source/FutebolAviao/Flight/BallPhysics.h Source/FutebolAviao/Flight/MatchRules.h Source/FutebolAviao/Flight/PlanePawn.h
git commit -m "tune: adjust ball and goal feel after playtest"
```

---

## Ao final da Fase 3

Existe um jogo: pilotar, gastar combustível pra alcançar a bola, acertar, fazer gol. É o primeiro momento em que dá pra perguntar "isso é divertido?" sobre o jogo inteiro, e não sobre uma mecânica isolada.

O que a Fase 4 herda pronto: `FMatchState` já separa placar por lado, e `ABallActor::Tick` já itera sobre todos os `APlanePawn` do mundo — quatro aviões entram sem mudar a colisão.

O que ainda não existe e a Fase 4 precisa: times (hoje o placar é por lado da arena, não por time), pickups de combustível, e o GameMode escolhendo spawn por time — o gancho `SetSpawnTransform` já está lá desde a Fase 2, sem ninguém chamando.
