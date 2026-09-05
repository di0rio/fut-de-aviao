# Fase 1 — Protótipo de Voo — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Um avião controlável em Unreal Engine (aceleração, curvas, altitude), sem bola nem combustível ainda — validar se pilotar é divertido antes de seguir para as próximas fases.

**Architecture:** A física de voo (velocidade, pitch/yaw/roll) é implementada como uma classe C++ pura, sem dependência da engine, testável isoladamente com um executável de teste standalone. Um `APawn` fino ("adapter") consome essa classe a cada `Tick`, converte o resultado em posição/rotação do ator via `FRotator`/`FVector`, e expõe os inputs clássicos de eixo (`Throttle`, `Pitch`, `Yaw`, `Roll`) configurados em `DefaultInput.ini`.

**Tech Stack:** Unreal Engine 5.8 (instalada em `C:\Program Files\Epic Games\UE_5.8`), C++17, sistema de Input clássico (Axis Mappings — não Enhanced Input, para manter tudo configurável via arquivos de texto sem precisar de assets binários criados no editor).

## Global Constraints

- Nome do projeto/módulo: `FutebolAviao` (sem acentos — precisa ser um identificador C++ válido).
- Engine associada: `5.8` (instalação local em `C:\Program Files\Epic Games\UE_5.8`).
- Plataforma alvo por agora: Win64 apenas.
- Escopo desta fase: só o avião controlável. Sem bola, sem combustível, sem outro time, sem rede — isso é responsabilidade das Fases 2 a 5 (ver `docs/superpowers/specs/2026-09-05-futebol-aviao-design.md`).
- Todo o código desta fase é C++ (sem Blueprints), para que cada task seja executável via edição de arquivos + compilação por linha de comando.
- **Pré-requisito bloqueante:** o compilador MSVC (Visual Studio Build Tools, workload "Game development with C++") precisa estar instalado antes de rodar a Task 2 em diante. A Task 1 não depende disso — só precisa de *algum* compilador C++ (MSVC ou um C++ genérico) disponível no PATH.

---

### Task 1: Física de voo pura (sem dependência da engine)

**Files:**
- Create: `Source/FutebolAviao/Flight/FlightPhysics.h`
- Create: `Source/FutebolAviao/Flight/FlightPhysics.cpp`
- Test: `Tools/FlightPhysicsTests/FlightPhysicsTests.cpp`

**Interfaces:**
- Produces: `struct FFlightPhysicsParams` (campos: `Acceleration`, `Deceleration`, `Drag`, `MaxSpeed`, `MinSpeed`, `PitchRateDegPerSec`, `YawRateDegPerSec`, `RollRateDegPerSec`, `MaxPitchDeg`, todos `float`); `struct FFlightPhysicsState` (campos: `Speed`, `PitchDeg`, `YawDeg`, `RollDeg`, todos `float`); `class FFlightPhysics` com construtor `explicit FFlightPhysics(const FFlightPhysicsParams& InParams)` e método `void Update(FFlightPhysicsState& State, float ThrottleInput, float PitchInput, float YawInput, float RollInput, float DeltaSeconds) const`. A Task 3 (PlanePawn) consome exatamente essa assinatura.

- [ ] **Step 1: Escrever os testes (vão falhar por enquanto)**

Criar `Tools/FlightPhysicsTests/FlightPhysicsTests.cpp`:

```cpp
#include "../../Source/FutebolAviao/Flight/FlightPhysics.h"
#include <cassert>
#include <cstdio>
#include <cmath>

static bool NearlyEqual(float A, float B, float Tolerance = 0.01f)
{
	return std::fabs(A - B) <= Tolerance;
}

static void Test_ThrottleAcceleratesSpeed()
{
	FFlightPhysicsParams Params;
	Params.Acceleration = 1000.f;
	Params.MaxSpeed = 10000.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;

	Physics.Update(State, /*Throttle*/ 1.f, 0.f, 0.f, 0.f, /*DeltaSeconds*/ 1.f);

	assert(NearlyEqual(State.Speed, 1000.f));
	printf("Test_ThrottleAcceleratesSpeed passed\n");
}

static void Test_SpeedClampsToMaxSpeed()
{
	FFlightPhysicsParams Params;
	Params.Acceleration = 100000.f;
	Params.MaxSpeed = 500.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;

	Physics.Update(State, 1.f, 0.f, 0.f, 0.f, 1.f);

	assert(NearlyEqual(State.Speed, 500.f));
	printf("Test_SpeedClampsToMaxSpeed passed\n");
}

static void Test_NoThrottleAppliesDrag()
{
	FFlightPhysicsParams Params;
	Params.Drag = 200.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;
	State.Speed = 1000.f;

	Physics.Update(State, 0.f, 0.f, 0.f, 0.f, 1.f);

	assert(NearlyEqual(State.Speed, 800.f));
	printf("Test_NoThrottleAppliesDrag passed\n");
}

static void Test_SpeedNeverGoesNegativeFromDrag()
{
	FFlightPhysicsParams Params;
	Params.Drag = 200.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;
	State.Speed = 50.f;

	Physics.Update(State, 0.f, 0.f, 0.f, 0.f, 1.f);

	assert(NearlyEqual(State.Speed, 0.f));
	printf("Test_SpeedNeverGoesNegativeFromDrag passed\n");
}

static void Test_PitchInputRotatesNoseAndClamps()
{
	FFlightPhysicsParams Params;
	Params.PitchRateDegPerSec = 90.f;
	Params.MaxPitchDeg = 85.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;

	Physics.Update(State, 0.f, 1.f, 0.f, 0.f, 1.f); // 90 deg pedido, clampa em 85
	assert(NearlyEqual(State.PitchDeg, 85.f));
	printf("Test_PitchInputRotatesNoseAndClamps passed\n");
}

static void Test_YawInputWrapsAround360()
{
	FFlightPhysicsParams Params;
	Params.YawRateDegPerSec = 350.f;
	FFlightPhysics Physics(Params);
	FFlightPhysicsState State;
	State.YawDeg = 20.f;

	Physics.Update(State, 0.f, 0.f, 1.f, 0.f, 1.f); // 20 + 350 = 370 -> wrap pra 10
	assert(NearlyEqual(State.YawDeg, 10.f));
	printf("Test_YawInputWrapsAround360 passed\n");
}

int main()
{
	Test_ThrottleAcceleratesSpeed();
	Test_SpeedClampsToMaxSpeed();
	Test_NoThrottleAppliesDrag();
	Test_SpeedNeverGoesNegativeFromDrag();
	Test_PitchInputRotatesNoseAndClamps();
	Test_YawInputWrapsAround360();
	printf("All tests passed\n");
	return 0;
}
```

- [ ] **Step 2: Rodar e confirmar que falha (arquivos de implementação ainda não existem)**

Abrir um "Developer Command Prompt for VS 2022" (ou rodar `"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"` antes) e rodar, na raiz do projeto:

```bash
cl /EHsc /std:c++17 /Fe:Tools\FlightPhysicsTests\FlightPhysicsTests.exe Tools\FlightPhysicsTests\FlightPhysicsTests.cpp
```

Esperado: FALHA de compilação — `FlightPhysics.h` não existe ainda (`cannot open source file`).

- [ ] **Step 3: Implementar `FlightPhysics.h`**

```cpp
#pragma once

struct FFlightPhysicsParams
{
	float Acceleration = 2000.f;
	float Deceleration = 1500.f;
	float Drag = 300.f;
	float MaxSpeed = 6000.f;
	float MinSpeed = 0.f;
	float PitchRateDegPerSec = 60.f;
	float YawRateDegPerSec = 90.f;
	float RollRateDegPerSec = 120.f;
	float MaxPitchDeg = 85.f;
};

struct FFlightPhysicsState
{
	float Speed = 0.f;
	float PitchDeg = 0.f;
	float YawDeg = 0.f;
	float RollDeg = 0.f;
};

class FFlightPhysics
{
public:
	FFlightPhysics() : Params() {}
	explicit FFlightPhysics(const FFlightPhysicsParams& InParams) : Params(InParams) {}

	// ThrottleInput, PitchInput, YawInput, RollInput sao esperados no intervalo [-1, 1].
	void Update(FFlightPhysicsState& State, float ThrottleInput, float PitchInput, float YawInput, float RollInput, float DeltaSeconds) const;

private:
	FFlightPhysicsParams Params;
};
```

- [ ] **Step 4: Implementar `FlightPhysics.cpp`**

```cpp
#include "FlightPhysics.h"

namespace
{
	float ClampValue(float Value, float Min, float Max)
	{
		if (Value < Min) return Min;
		if (Value > Max) return Max;
		return Value;
	}

	float WrapDegrees(float Degrees)
	{
		float Wrapped = Degrees;
		while (Wrapped >= 360.f) Wrapped -= 360.f;
		while (Wrapped < 0.f) Wrapped += 360.f;
		return Wrapped;
	}
}

void FFlightPhysics::Update(FFlightPhysicsState& State, float ThrottleInput, float PitchInput, float YawInput, float RollInput, float DeltaSeconds) const
{
	ThrottleInput = ClampValue(ThrottleInput, -1.f, 1.f);
	PitchInput = ClampValue(PitchInput, -1.f, 1.f);
	YawInput = ClampValue(YawInput, -1.f, 1.f);
	RollInput = ClampValue(RollInput, -1.f, 1.f);

	if (ThrottleInput > 0.f)
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

	State.Speed = ClampValue(State.Speed, Params.MinSpeed, Params.MaxSpeed);

	State.PitchDeg = ClampValue(State.PitchDeg + PitchInput * Params.PitchRateDegPerSec * DeltaSeconds, -Params.MaxPitchDeg, Params.MaxPitchDeg);
	State.YawDeg = WrapDegrees(State.YawDeg + YawInput * Params.YawRateDegPerSec * DeltaSeconds);
	State.RollDeg = WrapDegrees(State.RollDeg + RollInput * Params.RollRateDegPerSec * DeltaSeconds);
}
```

- [ ] **Step 5: Rodar os testes e confirmar que passam**

```bash
cl /EHsc /std:c++17 /Fe:Tools\FlightPhysicsTests\FlightPhysicsTests.exe Tools\FlightPhysicsTests\FlightPhysicsTests.cpp Source\FutebolAviao\Flight\FlightPhysics.cpp
Tools\FlightPhysicsTests\FlightPhysicsTests.exe
```

Esperado: as 6 linhas `Test_*passed` e, por fim, `All tests passed`.

- [ ] **Step 6: Commit**

```bash
git add Source/FutebolAviao/Flight/FlightPhysics.h Source/FutebolAviao/Flight/FlightPhysics.cpp Tools/FlightPhysicsTests/FlightPhysicsTests.cpp
git commit -m "feat: add pure flight physics module with standalone tests"
```

---

### Task 2: Scaffold do projeto Unreal

**Files:**
- Create: `FutebolAviao.uproject`
- Create: `Source/FutebolAviao.Target.cs`
- Create: `Source/FutebolAviaoEditor.Target.cs`
- Create: `Source/FutebolAviao/FutebolAviao.Build.cs`
- Create: `Source/FutebolAviao/FutebolAviao.h`
- Create: `Source/FutebolAviao/FutebolAviao.cpp`
- Create: `Source/FutebolAviao/FutebolAviaoGameModeBase.h`
- Create: `Source/FutebolAviao/FutebolAviaoGameModeBase.cpp`
- Create: `Config/DefaultEngine.ini`
- Create: `Config/DefaultInput.ini`

**Interfaces:**
- Consumes: nada (task independente da Task 1 em termos de compilação — o módulo do jogo simplesmente contém os arquivos da Task 1 dentro de si).
- Produces: módulo `FutebolAviao` compilável como alvo `FutebolAviaoEditor`; classe `AFutebolAviaoGameModeBase` (em `FutebolAviaoGameModeBase.h`) que a Task 3 vai modificar para apontar `DefaultPawnClass` para `APlanePawn`.

- [ ] **Step 1: Criar o arquivo de projeto `FutebolAviao.uproject`**

```json
{
	"FileVersion": 3,
	"EngineAssociation": "5.8",
	"Category": "",
	"Description": "",
	"Modules": [
		{
			"Name": "FutebolAviao",
			"Type": "Runtime",
			"LoadingPhase": "Default"
		}
	]
}
```

- [ ] **Step 2: Criar os arquivos de Target**

`Source/FutebolAviao.Target.cs`:

```csharp
using UnrealBuildTool;
using System.Collections.Generic;

public class FutebolAviaoTarget : TargetRules
{
	public FutebolAviaoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FutebolAviao");
	}
}
```

`Source/FutebolAviaoEditor.Target.cs`:

```csharp
using UnrealBuildTool;
using System.Collections.Generic;

public class FutebolAviaoEditorTarget : TargetRules
{
	public FutebolAviaoEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FutebolAviao");
	}
}
```

- [ ] **Step 3: Criar o módulo do jogo**

`Source/FutebolAviao/FutebolAviao.Build.cs`:

```csharp
using UnrealBuildTool;

public class FutebolAviao : ModuleRules
{
	public FutebolAviao(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
	}
}
```

`Source/FutebolAviao/FutebolAviao.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
```

`Source/FutebolAviao/FutebolAviao.cpp`:

```cpp
#include "FutebolAviao.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, FutebolAviao, "FutebolAviao");
```

- [ ] **Step 4: Criar o GameMode base**

`Source/FutebolAviao/FutebolAviaoGameModeBase.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FutebolAviaoGameModeBase.generated.h"

UCLASS()
class FUTEBOLAVIAO_API AFutebolAviaoGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFutebolAviaoGameModeBase();
};
```

`Source/FutebolAviao/FutebolAviaoGameModeBase.cpp`:

```cpp
#include "FutebolAviaoGameModeBase.h"

AFutebolAviaoGameModeBase::AFutebolAviaoGameModeBase()
{
}
```

- [ ] **Step 5: Configurar o GameMode global e os inputs clássicos**

`Config/DefaultEngine.ini`:

```ini
[/Script/EngineSettings.GameMapsSettings]
GlobalDefaultGameMode=/Script/FutebolAviao.FutebolAviaoGameModeBase
```

`Config/DefaultInput.ini`:

```ini
[/Script/Engine.InputSettings]
+AxisMappings=(AxisName="Throttle",Scale=1.000000,Key=W)
+AxisMappings=(AxisName="Throttle",Scale=-1.000000,Key=S)
+AxisMappings=(AxisName="Roll",Scale=-1.000000,Key=A)
+AxisMappings=(AxisName="Roll",Scale=1.000000,Key=D)
+AxisMappings=(AxisName="Pitch",Scale=1.000000,Key=MouseY)
+AxisMappings=(AxisName="Yaw",Scale=1.000000,Key=MouseX)
```

- [ ] **Step 6: Compilar o alvo Editor e confirmar que a build passa**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FutebolAviaoEditor Win64 Development -Project="C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -WaitMutex
```

Esperado: saída terminando em `Total execution time: ...` sem erros (`Result: Succeeded` ou equivalente). Se faltar o compilador MSVC, o erro vai indicar isso explicitamente — nesse caso, voltar ao pré-requisito do Build Tools antes de continuar.

- [ ] **Step 7: Commit**

```bash
git add FutebolAviao.uproject Source/FutebolAviao.Target.cs Source/FutebolAviaoEditor.Target.cs Source/FutebolAviao/FutebolAviao.Build.cs Source/FutebolAviao/FutebolAviao.h Source/FutebolAviao/FutebolAviao.cpp Source/FutebolAviao/FutebolAviaoGameModeBase.h Source/FutebolAviao/FutebolAviaoGameModeBase.cpp Config/DefaultEngine.ini Config/DefaultInput.ini
git commit -m "feat: scaffold Unreal project with compiling editor target"
```

---

### Task 3: `APlanePawn` — adapter entre a física pura e a engine

**Files:**
- Create: `Source/FutebolAviao/Flight/PlanePawn.h`
- Create: `Source/FutebolAviao/Flight/PlanePawn.cpp`
- Modify: `Source/FutebolAviao/FutebolAviaoGameModeBase.cpp` (define `DefaultPawnClass = APlanePawn::StaticClass()`)

**Interfaces:**
- Consumes: `FFlightPhysicsParams`, `FFlightPhysicsState`, `FFlightPhysics::Update(...)` da Task 1 (mesma assinatura, sem alterações).
- Produces: classe `APlanePawn` (em `PlanePawn.h`), usada pela Task 4 como o pawn padrão da partida — nenhuma outra task depende de métodos específicos dela além de já existir e ser instanciável em um nível.

- [ ] **Step 1: Criar `PlanePawn.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FlightPhysics.h"
#include "PlanePawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class FUTEBOLAVIAO_API APlanePawn : public APawn
{
	GENERATED_BODY()

public:
	APlanePawn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void HandleThrottleInput(float Value);
	void HandlePitchInput(float Value);
	void HandleYawInput(float Value);
	void HandleRollInput(float Value);

private:
	UPROPERTY(VisibleAnywhere, Category = "Plane")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Plane")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, Category = "Plane")
	UCameraComponent* CameraComponent;

	FFlightPhysics FlightPhysics;
	FFlightPhysicsState FlightState;

	float ThrottleInput = 0.f;
	float PitchInput = 0.f;
	float YawInput = 0.f;
	float RollInput = 0.f;
};
```

- [ ] **Step 2: Criar `PlanePawn.cpp`**

```cpp
#include "PlanePawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"

APlanePawn::APlanePawn()
	: FlightPhysics(FFlightPhysicsParams())
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMeshAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(ConeMeshAsset.Object);
		MeshComponent->SetRelativeScale3D(FVector(2.f, 1.f, 1.f));
		MeshComponent->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	}

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 800.f;
	SpringArmComponent->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
	SpringArmComponent->bDoCollisionTest = false;
	SpringArmComponent->bInheritPitch = false;
	SpringArmComponent->bInheritRoll = false;
	SpringArmComponent->bInheritYaw = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
}

void APlanePawn::BeginPlay()
{
	Super::BeginPlay();
}

void APlanePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FlightPhysics.Update(FlightState, ThrottleInput, PitchInput, YawInput, RollInput, DeltaSeconds);

	const FRotator NewRotation(FlightState.PitchDeg, FlightState.YawDeg, FlightState.RollDeg);
	SetActorRotation(NewRotation);

	const FVector Forward = NewRotation.Vector();
	AddActorWorldOffset(Forward * FlightState.Speed * DeltaSeconds, true);
}

void APlanePawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Throttle", this, &APlanePawn::HandleThrottleInput);
	PlayerInputComponent->BindAxis("Pitch", this, &APlanePawn::HandlePitchInput);
	PlayerInputComponent->BindAxis("Yaw", this, &APlanePawn::HandleYawInput);
	PlayerInputComponent->BindAxis("Roll", this, &APlanePawn::HandleRollInput);
}

void APlanePawn::HandleThrottleInput(float Value)
{
	ThrottleInput = Value;
}

void APlanePawn::HandlePitchInput(float Value)
{
	PitchInput = Value;
}

void APlanePawn::HandleYawInput(float Value)
{
	YawInput = Value;
}

void APlanePawn::HandleRollInput(float Value)
{
	RollInput = Value;
}
```

- [ ] **Step 3: Apontar o GameMode para `APlanePawn`**

Modificar `Source/FutebolAviao/FutebolAviaoGameModeBase.cpp`:

```cpp
#include "FutebolAviaoGameModeBase.h"
#include "Flight/PlanePawn.h"

AFutebolAviaoGameModeBase::AFutebolAviaoGameModeBase()
{
	DefaultPawnClass = APlanePawn::StaticClass();
}
```

- [ ] **Step 4: Compilar e confirmar que a build passa**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FutebolAviaoEditor Win64 Development -Project="C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -WaitMutex
```

Esperado: build sem erros. Se houver erro de linkagem faltando `SpringArmComponent`/`CameraComponent`, confirmar que os módulos `Engine` e `InputCore` (já listados no `Build.cs` da Task 2) cobrem essas dependências — ambos os componentes vivem no módulo `Engine`.

- [ ] **Step 5: Commit**

```bash
git add Source/FutebolAviao/Flight/PlanePawn.h Source/FutebolAviao/Flight/PlanePawn.cpp Source/FutebolAviao/FutebolAviaoGameModeBase.cpp
git commit -m "feat: add PlanePawn wiring flight physics to actor movement and input"
```

---

### Task 4: Nível de teste e playtest manual

**Files:**
- Nenhum arquivo de código — esta task é feita inteiramente na interface gráfica do Unreal Editor, porque criar um `.umap` (nível) exige o editor (não é um arquivo de texto editável diretamente).
- Modify (via editor, não via edição de texto): `Config/DefaultEngine.ini` (o editor adiciona `EditorStartupMap` e `GameDefaultMap` automaticamente ao salvar as configurações de Maps & Modes).

**Interfaces:**
- Consumes: `AFutebolAviaoGameModeBase` e `APlanePawn` da Task 3, já compilados.
- Produces: nada consumido por tasks futuras desta fase — esta é a entrega final da Fase 1.

- [ ] **Step 1: Abrir o projeto no Unreal Editor**

Abrir `FutebolAviao.uproject` (duplo clique, ou `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject"`). Se perguntado para compilar módulos ausentes, confirmar que sim.

- [ ] **Step 2: Criar o nível de teste**

File > New Level > escolher o template "Basic" (ou "Empty Level" + adicionar um `Floor`/`PlayerStart` manualmente). Adicionar um `PlayerStart` a uma altura razoável (ex: Z = 500) para o avião começar já no ar. Salvar como `Content/Maps/TestFlightMap.umap`.

- [ ] **Step 3: Definir esse nível como padrão do projeto**

Edit > Project Settings > Maps & Modes: setar "Editor Startup Map" e "Game Default Map" para `TestFlightMap`. Confirmar que "Default GameMode" mostra `FutebolAviaoGameModeBase` (herdado do `DefaultEngine.ini` da Task 2).

- [ ] **Step 4: Playtest manual**

Apertar Play (Alt+P ou o botão Play). Confirmar:
- W acelera, S desacelera/freia.
- Mouse (X/Y) vira o nariz do avião (yaw/pitch).
- A/D rola o avião (roll).
- O avião nunca trava nem desaparece do campo de visão da câmera (spring arm segue atrás dele).

Anotar qualquer ajuste de sensação desejado (ex: valores de `Acceleration`, `MaxSpeed`, taxas de rotação em `FFlightPhysicsParams`) — esses são só os defaults do construtor de `APlanePawn::FlightPhysics(FFlightPhysicsParams())`, fáceis de ajustar depois de sentir o voo.

- [ ] **Step 5: Commit**

```bash
git add Content/Maps/TestFlightMap.umap Config/DefaultEngine.ini
git commit -m "feat: add test flight level as project default map"
```

---

## Ao final da Fase 1

Prototype em mãos: um avião pilotável em 3D com aceleração, curvas e câmera de perseguição, pronto pra passar pro combustível/boost (Fase 2 da spec). Ajustes de sensação de voo (constantes em `FFlightPhysicsParams`) podem ser refinados a qualquer momento sem tocar no resto do código, já que ficam isolados na Task 1.
