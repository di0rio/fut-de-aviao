# Escala, estádio e tuning ao vivo — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reescalar o jogo para que a bola nunca fuja do avião, o campo comporte a velocidade, e cada número seja ajustável ao vivo pelo console — mais um estádio que dá para ler a 160 km/h.

**Architecture:** Os números mudam apenas nos headers puros, que continuam sendo a fonte única da verdade. A camada de tuning é feita de console variables que moram nos adapters e sobrescrevem campos apenas quando definidas, sem duplicar nenhum default. O estádio continua sendo gerado por script Python a partir de cubos da engine.

**Tech Stack:** Unreal Engine 5.8, C++17, sem Blueprints. Python via `PythonScriptPlugin` para gerar o nível.

## Global Constraints

- Módulo Unreal: `FutebolAviao`. Engine `5.8`, Win64, C++17. Sem Blueprints.
- **`PureMath.h`, `ArenaGeometry.h`, `FlightPhysics.{h,cpp}`, `FuelSystem.{h,cpp}`, `BallPhysics.{h,cpp}` e `MatchRules.{h,cpp}` não podem incluir NENHUM header da Unreal.** As suítes em `Tools/` compilam apenas `<Nome>Tests.cpp` mais `<Nome>.cpp`.
- Adapters (`APlanePawn`, `ABallActor`, `AFutebolAviaoGameModeBase`) são o único lugar com tipo da engine. **As CVars vivem lá, nunca nas classes puras.**
- **Nenhum default pode existir em dois lugares.** Duplicar `ArenaHalfX` entre dois headers foi o que produziu o bug do gol inalcançável na Fase 3. As CVars nascem em `-1` e só sobrescrevem quando definidas, justamente para não repetir o erro.
- A regra de design que amarra os números, da spec: **a bola nunca é mais rápida que o avião no boost, e é mais rápida que o avião em cruzeiro.** Se um número mudar no tuning, é essa relação que precisa continuar valendo.
- Fora do escopo: times, 2v2, pickups, rede, mecânica nova. Isto é reescala e tuning.
- Estilo: tabs, chaves Allman, comentários em português **sem acentos** (o código é estritamente ASCII).
- Fechar `UnrealEditor.exe` e `LiveCodingConsole.exe` antes de compilar ou de rodar o commandlet.

**Rodar os testes:**

```powershell
powershell -ExecutionPolicy Bypass -File Tools\run-tests.ps1
```

Descobre as 4 suítes, falha alto em suíte órfã, sai com código != 0 em qualquer falha.

**Compilar o editor:**

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FutebolAviaoEditor Win64 Development -Project="C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -WaitMutex
```

Sucesso é `Result: Succeeded`. `'vswhere.exe' is not recognized` é ruído esperado.

**Regenerar o nível:**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -run=pythonscript -script="C:\Users\cauad\Desktop\dev\jogao\Tools\LevelBuilder\BuildTestFlightMap.py" -unattended -nosplash -nop4
```

**Nunca confiar no `Python script executed successfully`.** Duas APIs da Unreal retornam sucesso sem fazer nada em modo headless (`EditorAssetLibrary.delete_asset` e `LevelEditorSubsystem.new_level`). Verificar sempre carregando o nível de volta e listando os atores.

---

### Task 1: Reescalar os números

**Files:**
- Modify: `Source/FutebolAviao/Flight/FlightPhysics.h`
- Modify: `Source/FutebolAviao/Flight/BallPhysics.h`
- Modify: `Source/FutebolAviao/Flight/ArenaGeometry.h`
- Modify: `Source/FutebolAviao/Flight/PlanePawn.h`, `PlanePawn.cpp`
- Modify: `Tools/LevelBuilder/BuildTestFlightMap.py`
- Modify: `Content/Maps/TestFlightMap.umap` (regenerado)

**Interfaces:**
- Consumes: nada.
- Produces: os defaults novos que as Tasks 2 e 4 assumem. Nenhuma assinatura muda.

Esta task mexe só em valores. Nenhuma regra nova, nenhuma função nova.

- [ ] **Step 1: Novos defaults de voo**

Em `Source/FutebolAviao/Flight/FlightPhysics.h`, dentro de `FFlightPhysicsParams`, trocar os valores (mantendo os comentários existentes atualizados):

```cpp
	float Acceleration = 1950.f;  // 0 -> maxima em ~2.3s, mesmo tempo de antes
	float Deceleration = 1650.f;  // S freia de verdade: maxima -> parado em ~2.7s
	float Drag = 525.f;           // soltar o acelerador desacelera em ~6.5s
	float MaxSpeed = 4500.f;      // 45 m/s: atravessar o campo novo leva ~8.9s
	float BoostMaxSpeed = 6500.f; // 65 m/s: acima do teto da bola, pra poder alcanca-la
	float BoostAcceleration = 3750.f;
```

`MinSpeed`, `PitchRateDegPerSec`, `YawRateDegPerSec`, `RollRateDegPerSec` e `MaxPitchDeg` **não mudam**.

E trocar o default da inércia:

```cpp
	float VelocityAlignPerSec = 25.f;
```

atualizando o comentário acima dele para dizer que ~25 dá um atraso de ~4 graus numa curva a taxa máxima, e que valores altos (ex: 1000) desligam a inércia por completo.

- [ ] **Step 2: Novos defaults da bola**

Em `Source/FutebolAviao/Flight/BallPhysics.h`, dentro de `FBallPhysicsParams`:

```cpp
	float Drag = 0.20f;             // fracao da velocidade perdida por segundo
	float Radius = 400.f;           // 8m de diametro: visivel a 400m de distancia
	float MaxSpeed = 6000.f;        // 60 m/s: abaixo do boost (65), acima do cruzeiro (45)
	float HitTransfer = 1.1f;       // acerto em cruzeiro produz 5250, abaixo do teto
	float MinKick = 300.f;          // raspao ainda mexe na bola, sem catapultar
```

`Gravity` e `Restitution` **não mudam**.

- [ ] **Step 3: Nova arena**

Em `Source/FutebolAviao/Flight/ArenaGeometry.h`:

```cpp
	float ArenaHalfX = 20000.f;      // 400m de comprimento
	float ArenaHalfY = 12000.f;      // 240m de largura
	float ArenaCeilingZ = 12000.f;   // 120m de altura
	float GoalHalfWidthY = 3000.f;   // boca de 60m
	float GoalHeightZ = 4000.f;      // 40m de altura
```

- [ ] **Step 4: Escala visual do avião e da câmera**

Em `Source/FutebolAviao/Flight/PlanePawn.cpp`, no construtor, trocar a escala da malha:

```cpp
		MeshComponent->SetRelativeScale3D(FVector(12.f, 6.f, 6.f));
```

e o braço da câmera:

```cpp
	SpringArmComponent->TargetArmLength = 2500.f;
```

Em `Source/FutebolAviao/Flight/PlanePawn.h`, o raio de colisão:

```cpp
	UFUNCTION(BlueprintCallable, Category = "Plane")
	float GetCollisionRadius() const { return 600.f; }
```

- [ ] **Step 5: Rodar os testes**

```powershell
powershell -ExecutionPolicy Bypass -File Tools\run-tests.ps1
```

**Se algum teste falhar, NÃO ajustar o valor esperado dele.** Uma falha aqui significa que o teste dependia de um default sem declarar — o conserto é o teste passar a fixar explicitamente o parâmetro de que precisa, exatamente como foi feito na Fase 1 quando `MinSpeed` mudou. Registrar no relatório qual teste tinha a dependência implícita.

- [ ] **Step 6: Novas constantes no gerador de nível**

Em `Tools/LevelBuilder/BuildTestFlightMap.py`, atualizar o bloco de constantes para casar com `ArenaGeometry.h` (o aviso que já existe acima delas continua valendo):

```python
ARENA_HALF_X = 20000.0
ARENA_HALF_Y = 12000.0
ARENA_CEILING_Z = 12000.0
GOAL_HALF_WIDTH_Y = 3000.0
GOAL_HEIGHT_Z = 4000.0

PLAYER_START_Z = 1500.0
PLAYER_START_X = 16000.0
```

E o piso, que precisa cobrir os 400m × 240m (o cubo da engine tem 100 de lado):

```python
FLOOR_SCALE = unreal.Vector(ARENA_HALF_X * 2 / 100.0, ARENA_HALF_Y * 2 / 100.0, 1.0)
```

- [ ] **Step 7: Regenerar e verificar o nível**

Rodar o commandlet das Global Constraints. Depois **verificar carregando o nível de volta** — criar um script temporário fora do repositório com:

```python
import unreal

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors_ss = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
lines = ["load=%s" % level_editor.load_level("/Game/Maps/TestFlightMap")]
for a in actors_ss.get_all_level_actors():
    loc = a.get_actor_location()
    s = a.get_actor_scale3d()
    lines.append("%-24s loc=(%.0f,%.0f,%.0f) scale=(%.1f,%.1f,%.1f)" % (
        a.get_actor_label(), loc.x, loc.y, loc.z, s.x, s.y, s.z))
open(r"C:\Users\cauad\AppData\Local\Temp\verify_out.txt", "w").write("\n".join(lines))
```

rodá-lo pelo mesmo commandlet e conferir que as paredes estão em ±20000/±12000, o teto em 12000, e os `PlayerStart` em ±16000. Colar o dump no relatório.

- [ ] **Step 8: Compilar o editor**

Esperado: `Result: Succeeded`.

- [ ] **Step 9: Commit**

```bash
git add Source/FutebolAviao/Flight/FlightPhysics.h Source/FutebolAviao/Flight/BallPhysics.h Source/FutebolAviao/Flight/ArenaGeometry.h Source/FutebolAviao/Flight/PlanePawn.h Source/FutebolAviao/Flight/PlanePawn.cpp Tools/LevelBuilder/BuildTestFlightMap.py Content/Maps/TestFlightMap.umap
git commit -m "tune: rescale plane, ball and arena so the ball never outruns the plane"
```

---

### Task 2: Tuning ao vivo por console variables

**Files:**
- Create: `Source/FutebolAviao/Tuning/TuningCVars.h`
- Modify: `Source/FutebolAviao/Flight/FlightPhysics.h`
- Modify: `Source/FutebolAviao/Flight/FuelSystem.h`
- Modify: `Source/FutebolAviao/Flight/BallPhysics.h`
- Modify: `Source/FutebolAviao/Flight/PlanePawn.h`, `PlanePawn.cpp`
- Modify: `Source/FutebolAviao/Flight/BallActor.h`, `BallActor.cpp`
- Modify: `Tools/FlightPhysicsTests/FlightPhysicsTests.cpp`

**Interfaces:**
- Consumes: os defaults da Task 1.
- Produces: `Tuning::Apply(const TAutoConsoleVariable<float>&, float&)`; `FFlightPhysics::SetParams`, `FFuelSystem::SetParams`, `FBallPhysics::SetParams`, e os respectivos `GetParams`. As CVars listadas ao fim da task.

As classes puras hoje recebem params só no construtor. Para tunar ao vivo elas precisam de um setter — que continua sendo C++ puro, sem tocar na fronteira.

- [ ] **Step 1: Escrever o teste do setter (vai falhar)**

Inserir antes de `int main()` em `Tools/FlightPhysicsTests/FlightPhysicsTests.cpp`:

```cpp
static void Test_SetParamsChangesBehaviourAtRuntime()
{
	// O tuning ao vivo depende disso: trocar os params de uma instancia ja
	// construida tem que mudar o resultado do proximo Update.
	FFlightPhysicsParams Slow;
	Slow.MinSpeed = 0.f;
	Slow.Acceleration = 1000.f;
	Slow.MaxSpeed = 100000.f;
	FFlightPhysics Physics(Slow);

	FFlightPhysicsState State;
	Physics.Update(State, 1.f, 0.f, 0.f, 0.f, false, 1.f);
	assert(NearlyEqual(FFlightPhysics::GetSpeed(State), 1000.f));

	FFlightPhysicsParams Fast = Slow;
	Fast.Acceleration = 5000.f;
	Physics.SetParams(Fast);

	Physics.Update(State, 1.f, 0.f, 0.f, 0.f, false, 1.f);
	assert(NearlyEqual(FFlightPhysics::GetSpeed(State), 6000.f));
	printf("Test_SetParamsChangesBehaviourAtRuntime passed\n");
}
```

E registrar em `main()`, depois da última chamada de teste existente:

```cpp
	Test_SetParamsChangesBehaviourAtRuntime();
```

- [ ] **Step 2: Rodar e confirmar que falha**

Esperado: erro de compilação `'SetParams': is not a member of 'FFlightPhysics'`.

- [ ] **Step 3: Adicionar os setters às três classes puras**

Em `Source/FutebolAviao/Flight/FlightPhysics.h`, na seção `public:` de `FFlightPhysics`:

```cpp
	// Permite tuning ao vivo: o adapter sobrescreve os params por console
	// variable sem reconstruir o objeto. Continua C++ puro.
	void SetParams(const FFlightPhysicsParams& InParams) { Params = InParams; }
	const FFlightPhysicsParams& GetParams() const { return Params; }
```

Em `Source/FutebolAviao/Flight/FuelSystem.h`, na seção `public:` de `FFuelSystem`:

```cpp
	void SetParams(const FFuelParams& InParams) { Params = InParams; }
	const FFuelParams& GetParams() const { return Params; }
```

`FBallPhysics` já tem `GetParams`; adicionar ao lado dele em `Source/FutebolAviao/Flight/BallPhysics.h`:

```cpp
	void SetParams(const FBallPhysicsParams& InParams) { Params = InParams; }
```

- [ ] **Step 4: Rodar e confirmar que passa**

- [ ] **Step 5: Commit**

```bash
git add Source/FutebolAviao/Flight/FlightPhysics.h Source/FutebolAviao/Flight/FuelSystem.h Source/FutebolAviao/Flight/BallPhysics.h Tools/FlightPhysicsTests/FlightPhysicsTests.cpp
git commit -m "feat: let pure classes take new params at runtime"
```

- [ ] **Step 6: Criar o helper de tuning**

Criar `Source/FutebolAviao/Tuning/TuningCVars.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

// Camada de tuning ao vivo. Vive no lado da engine de proposito: as classes
// puras nao podem incluir header da Unreal.
//
// Convencao: cada CVar nasce em -1 ("nao definida") e so sobrescreve o campo
// quando o valor for >= 0. Assim os headers puros continuam sendo a fonte
// unica da verdade dos defaults -- nenhum numero e repetido aqui. Duplicar
// default entre dois lugares foi o que produziu o bug do gol inalcancavel na
// Fase 3, e esta convencao existe pra nao repetir isso.
namespace Tuning
{
	inline void Apply(const TAutoConsoleVariable<float>& Var, float& Field)
	{
		const float Value = Var.GetValueOnGameThread();
		if (Value >= 0.f)
		{
			Field = Value;
		}
	}
}
```

- [ ] **Step 7: CVars do avião e do combustível**

Em `Source/FutebolAviao/Flight/PlanePawn.cpp`, adicionar o include e o bloco de CVars logo depois dos includes existentes:

```cpp
#include "../Tuning/TuningCVars.h"

namespace
{
	TAutoConsoleVariable<float> CVarPlaneMaxSpeed(TEXT("fa.Plane.MaxSpeed"), -1.f, TEXT("Velocidade maxima em cruzeiro, cm/s. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneBoostMaxSpeed(TEXT("fa.Plane.BoostMaxSpeed"), -1.f, TEXT("Teto de velocidade no boost, cm/s. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneAcceleration(TEXT("fa.Plane.Acceleration"), -1.f, TEXT("Aceleracao, cm/s2. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneBoostAcceleration(TEXT("fa.Plane.BoostAcceleration"), -1.f, TEXT("Aceleracao no boost, cm/s2. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneDrag(TEXT("fa.Plane.Drag"), -1.f, TEXT("Arrasto com acelerador solto, cm/s2. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneTurnRate(TEXT("fa.Plane.TurnRate"), -1.f, TEXT("Taxa de pitch E yaw juntos, graus/s. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneVelocityAlign(TEXT("fa.Plane.VelocityAlign"), -1.f, TEXT("Quao rapido a velocidade persegue o nariz. Valor alto (1000) desliga a inercia. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneCollisionRadius(TEXT("fa.Plane.CollisionRadius"), -1.f, TEXT("Raio esferico do aviao pra colisao com a bola. -1 usa o default."));

	TAutoConsoleVariable<float> CVarFuelTankCapacity(TEXT("fa.Fuel.TankCapacity"), -1.f, TEXT("Tamanho do tanque. -1 usa o default."));
	TAutoConsoleVariable<float> CVarFuelBoostDrain(TEXT("fa.Fuel.BoostDrain"), -1.f, TEXT("Consumo do boost por segundo. -1 usa o default."));
	TAutoConsoleVariable<float> CVarFuelRegen(TEXT("fa.Fuel.Regen"), -1.f, TEXT("Regeneracao passiva por segundo. -1 usa o default."));
	TAutoConsoleVariable<float> CVarFuelRespawnSeconds(TEXT("fa.Fuel.RespawnSeconds"), -1.f, TEXT("Segundos fora da jogada depois de explodir. -1 usa o default."));
}
```

Em `Source/FutebolAviao/Flight/PlanePawn.h`, na seção `private:`, declarar o método e o raio ajustável:

```cpp
	void ApplyTuningCVars();

	// Um unico lugar define o default; ApplyTuningCVars restaura a partir dele
	// todo tick, pra que voltar a CVar pra -1 volte de fato ao default.
	static constexpr float DefaultCollisionRadius = 600.f;
	float CollisionRadius = DefaultCollisionRadius;
```

e trocar o getter do raio, que agora lê o campo:

```cpp
	UFUNCTION(BlueprintCallable, Category = "Plane")
	float GetCollisionRadius() const { return CollisionRadius; }
```

Em `Source/FutebolAviao/Flight/PlanePawn.cpp`, implementar:

```cpp
void APlanePawn::ApplyTuningCVars()
{
	// Parte SEMPRE dos defaults puros, nunca dos params atuais. Se lesse os
	// atuais, um valor setado por CVar ficaria grudado pra sempre: voltar a
	// CVar pra -1 nao teria como restaurar o default.
	FFlightPhysicsParams FlightParams;
	Tuning::Apply(CVarPlaneMaxSpeed, FlightParams.MaxSpeed);
	Tuning::Apply(CVarPlaneBoostMaxSpeed, FlightParams.BoostMaxSpeed);
	Tuning::Apply(CVarPlaneAcceleration, FlightParams.Acceleration);
	Tuning::Apply(CVarPlaneBoostAcceleration, FlightParams.BoostAcceleration);
	Tuning::Apply(CVarPlaneDrag, FlightParams.Drag);
	Tuning::Apply(CVarPlaneVelocityAlign, FlightParams.VelocityAlignPerSec);

	// TurnRate mexe nos dois eixos juntos: eles sao mantidos iguais de
	// proposito, pra mirar em 3D ser simetrico.
	Tuning::Apply(CVarPlaneTurnRate, FlightParams.PitchRateDegPerSec);
	Tuning::Apply(CVarPlaneTurnRate, FlightParams.YawRateDegPerSec);
	FlightPhysics.SetParams(FlightParams);

	FFuelParams TunedFuel;
	Tuning::Apply(CVarFuelTankCapacity, TunedFuel.TankCapacity);
	Tuning::Apply(CVarFuelBoostDrain, TunedFuel.BoostDrainPerSec);
	Tuning::Apply(CVarFuelRegen, TunedFuel.PassiveRegenPerSec);
	Tuning::Apply(CVarFuelRespawnSeconds, TunedFuel.RespawnSeconds);
	FuelSystem.SetParams(TunedFuel);

	CollisionRadius = DefaultCollisionRadius;
	Tuning::Apply(CVarPlaneCollisionRadius, CollisionRadius);
}
```

E chamar como **primeira linha** de `APlanePawn::Tick`, logo depois de `Super::Tick(DeltaSeconds);`:

```cpp
	ApplyTuningCVars();
```

- [ ] **Step 8: CVars da bola**

Em `Source/FutebolAviao/Flight/BallActor.cpp`, adicionar o include e o bloco de CVars junto dos includes:

```cpp
#include "../Tuning/TuningCVars.h"
```

e dentro do `namespace { ... }` anônimo que já existe no arquivo:

```cpp
	TAutoConsoleVariable<float> CVarBallMaxSpeed(TEXT("fa.Ball.MaxSpeed"), -1.f, TEXT("Teto de velocidade da bola, cm/s. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallGravity(TEXT("fa.Ball.Gravity"), -1.f, TEXT("Gravidade da bola, cm/s2. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallDrag(TEXT("fa.Ball.Drag"), -1.f, TEXT("Fracao da velocidade perdida por segundo. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallRestitution(TEXT("fa.Ball.Restitution"), -1.f, TEXT("Quanto da velocidade sobra depois de quicar. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallHitTransfer(TEXT("fa.Ball.HitTransfer"), -1.f, TEXT("Quanto da velocidade do aviao vira impulso. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallMinKick(TEXT("fa.Ball.MinKick"), -1.f, TEXT("Impulso minimo de um toque de raspao. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallRadius(TEXT("fa.Ball.Radius"), -1.f, TEXT("Raio da bola. Reescala a malha junto. -1 usa o default."));
```

Em `Source/FutebolAviao/Flight/BallActor.h`, na seção `private:`:

```cpp
	void ApplyTuningCVars();
```

Em `BallActor.cpp`, implementar:

```cpp
void ABallActor::ApplyTuningCVars()
{
	// Como no aviao: parte dos defaults puros, nunca dos params atuais, senao
	// um valor de CVar nunca mais sai depois de setado.
	const float PreviousRadius = BallPhysics.GetParams().Radius;
	FBallPhysicsParams Params;

	Tuning::Apply(CVarBallMaxSpeed, Params.MaxSpeed);
	Tuning::Apply(CVarBallGravity, Params.Gravity);
	Tuning::Apply(CVarBallDrag, Params.Drag);
	Tuning::Apply(CVarBallRestitution, Params.Restitution);
	Tuning::Apply(CVarBallHitTransfer, Params.HitTransfer);
	Tuning::Apply(CVarBallMinKick, Params.MinKick);
	Tuning::Apply(CVarBallRadius, Params.Radius);

	BallPhysics.SetParams(Params);

	// A malha precisa acompanhar o raio, senao o visual e a colisao divergem
	// e o jogador passa a mirar num lugar onde a bola nao esta.
	if (Params.Radius != PreviousRadius && MeshComponent)
	{
		const float Scale = Params.Radius / 50.f;
		MeshComponent->SetRelativeScale3D(FVector(Scale, Scale, Scale));
	}
}
```

E chamar como primeira linha de `ABallActor::Tick`, logo depois de `Super::Tick(DeltaSeconds);`:

```cpp
	ApplyTuningCVars();
```

- [ ] **Step 9: Compilar o editor**

Esperado: `Result: Succeeded`.

- [ ] **Step 10: Rodar os testes**

Esperado: as 4 suítes verdes. Nenhum valor esperado muda — as CVars nascem desativadas, então o comportamento default é idêntico.

- [ ] **Step 11: Commit**

```bash
git add Source/FutebolAviao/Tuning/TuningCVars.h Source/FutebolAviao/Flight/PlanePawn.h Source/FutebolAviao/Flight/PlanePawn.cpp Source/FutebolAviao/Flight/BallActor.h Source/FutebolAviao/Flight/BallActor.cpp
git commit -m "feat: add console variables for live tuning without a rebuild"
```

---

### Task 3: O estádio

**Files:**
- Modify: `Tools/LevelBuilder/BuildTestFlightMap.py`
- Modify: `Content/Maps/TestFlightMap.umap` (regenerado)

**Interfaces:**
- Consumes: as dimensões da Task 1.
- Produces: o nível final. Nada de código consome isto.

O objetivo é **legibilidade**: a 160 km/h numa caixa vazia de 400m não dá para saber para onde se está voando. O campo tem que responder de relance onde é o gol, onde acaba o campo, e para que lado se está virado.

- [ ] **Step 1: Verificação automática da duplicação C++/Python**

O script duplica as dimensões de `ArenaGeometry.h` porque é um programa separado. Hoje isso é só um comentário de aviso. Com o estádio derivando mais coisas das mesmas medidas, passa a valer falhar alto.

Adicionar em `Tools/LevelBuilder/BuildTestFlightMap.py`, depois do bloco de constantes:

```python
import os
import re

def check_matches_cpp():
    """Falha alto se as constantes daqui divergirem de ArenaGeometry.h.

    A duplicacao nao tem como ser eliminada (Python e C++ sao programas
    separados), entao o proximo melhor e detecta-la na hora em vez de
    descobrir jogando que a bola quica no nada.
    """
    header = os.path.join(os.path.dirname(__file__), "..", "..",
                          "Source", "FutebolAviao", "Flight", "ArenaGeometry.h")
    with open(header, "r") as f:
        text = f.read()

    expected = {
        "ArenaHalfX": ARENA_HALF_X,
        "ArenaHalfY": ARENA_HALF_Y,
        "ArenaCeilingZ": ARENA_CEILING_Z,
        "GoalHalfWidthY": GOAL_HALF_WIDTH_Y,
        "GoalHeightZ": GOAL_HEIGHT_Z,
    }

    for name, value in expected.items():
        match = re.search(r"float\s+%s\s*=\s*([0-9.]+)f" % name, text)
        if not match:
            raise RuntimeError("Nao achei %s em ArenaGeometry.h" % name)
        found = float(match.group(1))
        if abs(found - value) > 0.001:
            raise RuntimeError(
                "%s divergente: Python tem %.1f, ArenaGeometry.h tem %.1f. "
                "Alinhe os dois antes de regenerar o mapa." % (name, value, found))
```

E chamar como primeira linha de `build()`:

```python
    check_matches_cpp()
```

- [ ] **Step 2: Provar que a verificação pega a divergência**

Mudar temporariamente `ARENA_HALF_X` no Python para `19000.0`, rodar o commandlet, e confirmar que ele falha com a mensagem de divergência em vez de gerar um mapa errado. Restaurar para `20000.0` e confirmar que volta a gerar. Reportar o texto exato do erro.

- [ ] **Step 3: Arquibancadas nas laterais**

As paredes retas das laterais longas viram arquibancadas inclinadas. **A inclinação é puramente visual** — a física da bola quica numa caixa alinhada aos eixos e continua quicando no plano reto em `±ArenaHalfY`. As arquibancadas ficam *atrás* desse limite, nunca em cima dele.

Substituir o laço que hoje cria `Parede_Norte` / `Parede_Sul` por:

```python
    # Arquibancadas: tres degraus inclinados por lado, subindo pra fora do campo.
    # Puramente visual -- o limite fisico continua sendo o plano reto em
    # +-ARENA_HALF_Y (ver BounceAxis em BallPhysics.cpp).
    for side, sign in (("Norte", 1.0), ("Sul", -1.0)):
        for tier in range(3):
            tier_height = 2000.0 + tier * 2500.0
            tier_offset = tier * 1800.0
            spawn_box(actors, "Arquibancada_%s_%d" % (side, tier),
                      unreal.Vector(0.0,
                                    sign * (ARENA_HALF_Y + tier_offset),
                                    tier_height / 2.0),
                      unreal.Vector(ARENA_HALF_X * 2 / 100.0, 8.0, tier_height / 100.0))
```

- [ ] **Step 4: Marcações no chão**

Blocos finos e baixos, só para dar referência de posição e velocidade. Ficam 20 unidades acima do piso para não brigar com ele no z-fighting:

```python
    MARK_Z = 70.0
    MARK_THICKNESS = 0.4   # em unidades de cubo (100), ou seja 40 unidades

    # Linha de meio-campo, cruzando a largura.
    spawn_box(actors, "Marca_MeioCampo",
              unreal.Vector(0.0, 0.0, MARK_Z),
              unreal.Vector(MARK_THICKNESS, ARENA_HALF_Y * 2 / 100.0, 0.2))

    # Circulo central aproximado por 16 blocos, raio de 4000.
    import math
    for i in range(16):
        angle = (2.0 * math.pi * i) / 16.0
        spawn_box(actors, "Marca_Circulo_%d" % i,
                  unreal.Vector(math.cos(angle) * 4000.0, math.sin(angle) * 4000.0, MARK_Z),
                  unreal.Vector(6.0, MARK_THICKNESS, 0.2))

    # Grandes areas: retangulo aberto na frente de cada gol.
    AREA_DEPTH = 6000.0
    AREA_HALF_WIDTH = GOAL_HALF_WIDTH_Y + 3000.0
    for side, sign in (("Oeste", -1.0), ("Leste", 1.0)):
        # Linha paralela a linha de fundo.
        spawn_box(actors, "Marca_Area_%s_Frente" % side,
                  unreal.Vector(sign * (ARENA_HALF_X - AREA_DEPTH), 0.0, MARK_Z),
                  unreal.Vector(MARK_THICKNESS, AREA_HALF_WIDTH * 2 / 100.0, 0.2))
        # Duas linhas perpendiculares fechando a area.
        for edge, edge_sign in (("A", 1.0), ("B", -1.0)):
            spawn_box(actors, "Marca_Area_%s_%s" % (side, edge),
                      unreal.Vector(sign * (ARENA_HALF_X - AREA_DEPTH / 2.0),
                                    edge_sign * AREA_HALF_WIDTH, MARK_Z),
                      unreal.Vector(AREA_DEPTH / 100.0, MARK_THICKNESS, 0.2))
```

- [ ] **Step 5: Torres de canto e pontas com silhuetas diferentes**

As torres de canto ancoram a orientação; as pontas diferentes respondem "para que lado estou virado" sem depender de cor:

```python
    # Torres nos quatro cantos: referencia de orientacao a distancia.
    for x_side, x_sign in (("O", -1.0), ("L", 1.0)):
        for y_side, y_sign in (("N", 1.0), ("S", -1.0)):
            spawn_box(actors, "Torre_%s%s" % (x_side, y_side),
                      unreal.Vector(x_sign * ARENA_HALF_X, y_sign * ARENA_HALF_Y, 9000.0),
                      unreal.Vector(12.0, 12.0, 180.0))

    # Silhuetas distintas por ponta: e o que diz de relance pra que lado voce
    # esta voando. Oeste = duas torres altas e finas; Leste = um bloco largo e
    # baixo. Sem cor de proposito -- exigiria instancia de material.
    for edge_sign in (1.0, -1.0):
        spawn_box(actors, "Marco_Oeste_%d" % int(edge_sign),
                  unreal.Vector(-(ARENA_HALF_X + 3000.0), edge_sign * 4000.0, 11000.0),
                  unreal.Vector(15.0, 15.0, 220.0))

    spawn_box(actors, "Marco_Leste",
              unreal.Vector(ARENA_HALF_X + 3000.0, 0.0, 3000.0),
              unreal.Vector(20.0, 140.0, 60.0))
```

- [ ] **Step 6: Regenerar e verificar**

Rodar o commandlet, depois **carregar o nível de volta** com o script de verificação da Task 1 Step 7 e conferir:

- o teto existe, em Z = 12000
- as arquibancadas estão em `|Y| >= 12000` (fora do limite físico, nunca dentro)
- as marcações estão em Z ≈ 70, baixas
- as duas pontas têm marcos de alturas diferentes

Colar o dump completo no relatório.

- [ ] **Step 7: Rodar os testes e compilar o editor**

Nada de C++ mudou nesta task, mas rodar as duas coisas confirma que nada regrediu.

- [ ] **Step 8: Commit**

```bash
git add Tools/LevelBuilder/BuildTestFlightMap.py Content/Maps/TestFlightMap.umap
git commit -m "feat: build a legible stadium and verify the C++/Python arena sync"
```

---

### Task 4: Playtest

**Files:** nenhum, a menos que o playtest peça ajuste.

- [ ] **Step 1: Abrir e jogar**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject"
```

- [ ] **Step 2: Conferir o que a reescala prometeu**

- Dá para perseguir o próprio chute? (era impossível antes)
- O boost tem função — é preciso boostar para alcançar uma bola bem batida?
- O campo comporta a velocidade sem parede na cara toda hora?
- Dá para saber para que lado se está voando sem parar para pensar?
- O avião ainda escorrega?

- [ ] **Step 3: Tunar ao vivo, sem sair do jogo**

Abrir o console com `~` durante o Play. As mais prováveis:

| Se... | Comando |
|---|---|
| ainda escorrega | `fa.Plane.VelocityAlign 1000` (desliga a inércia) |
| ainda rápido demais | `fa.Plane.MaxSpeed 3500` |
| a bola ainda foge | `fa.Ball.MaxSpeed 4500` |
| o toque manda longe demais | `fa.Ball.HitTransfer 0.8` |
| a bola parece leve | `fa.Ball.Gravity 1600` |
| difícil acertar a bola | `fa.Plane.CollisionRadius 900` |
| o campo parece grande demais | anotar — arena não é CVar, exige regenerar o mapa |

- [ ] **Step 4: Gravar os valores que ficaram bons**

Os que agradarem viram os novos defaults nos headers puros. **Não deixar valor bom só na CVar** — CVar não persiste, e no próximo Play tudo volta ao default.

- [ ] **Step 5: Commit**

```bash
git add Source/FutebolAviao/Flight/FlightPhysics.h Source/FutebolAviao/Flight/BallPhysics.h
git commit -m "tune: adopt the values that felt right in playtest"
```

---

## Ao final

O jogo passa a ter uma escala coerente, com uma regra explícita amarrando os três números que importam, e um campo que se lê a 160 km/h. E o ciclo de tuning deixa de custar um rebuild por tentativa — o que muda quantas ideias dá para testar numa sessão.

A arena continua fora do alcance das CVars de propósito: mudá-la exige regenerar o nível, e um mapa fora de sincronia com a física é pior que um mapa do tamanho errado.
