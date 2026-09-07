# Modelos de avião — Plano de implementação

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Trocar o cone que representa o avião por três modelos montados com primitivas da engine, dimensionados em proporção à bola, com o visual separado da colisão.

**Architecture:** Uma classe pura `PlaneModel` descreve cada avião como uma lista de peças medidas **em diâmetros de bola**, sem nenhum header da Unreal, e é coberta por uma suíte standalone. Um adapter do lado da engine (`PlaneMeshBuilder`) transforma essa lista em `UStaticMeshComponent`s puramente cosméticos, enquanto uma `USphereComponent` passa a ser a única colisão do avião.

**Tech Stack:** C++17, Unreal Engine 5.8, primitivas de `/Engine/BasicShapes`, suítes standalone compiladas com `cl.exe` do Visual Studio Build Tools via `Tools/run-tests.ps1`.

**Spec:** [`docs/superpowers/specs/2026-09-06-modelos-de-aviao-design.md`](../specs/2026-09-06-modelos-de-aviao-design.md)

## Global Constraints

- **As classes puras não podem incluir nenhum header da Unreal.** `PlaneModel.h` e `PlaneModel.cpp` incluem só `PureMath.h` e headers da std. É essa fronteira que permite as suítes standalone e a prediction da Fase 5.
- **Todas as medidas de `PlaneModel` são em diâmetros de bola**, nunca em centímetros. A conversão acontece num único lugar (as funções `PartLocation` / `PartMeshScale` / `CollisionRadiusFor`).
- **Sem STL e sem alocação no código puro.** Arrays de tamanho fixo devolvidos por valor, como o resto de `Flight/`.
- **Toda suíte nova em `Tools/<Nome>Tests/` precisa de `Source/FutebolAviao/Flight/<Nome>.cpp` para parear**, senão `run-tests.ps1` a reporta como órfã e derruba o exit code.
- **Nenhum asset novo, nenhum binário no git.** Só primitivas de `/Engine/BasicShapes` e o material `/Engine/BasicShapes/BasicShapeMaterial`.
- **Nenhum número de gameplay muda.** Física de voo, física da bola e regras de gol ficam intocadas. Com a bola de hoje (`Radius = 400`), o raio de colisão derivado tem que dar exatamente `600` — o valor que já está no código.
- Comentários e mensagens de commit em português sem acentos, seguindo o que já está no repositório.

**Desvio de nomenclatura em relação à spec:** a spec escreve `namespace FPlaneModel`. O prefixo `F` é para structs e classes, não para namespaces. Este plano usa `namespace PlaneModel`. O arquivo continua sendo `PlaneModel.cpp`, que é o que o pareamento do `run-tests.ps1` exige.

---

### Task 1: `PlaneModel` puro com o modelo Arcade e sua suíte

**Files:**
- Create: `Source/FutebolAviao/Flight/PlaneModel.h`
- Create: `Source/FutebolAviao/Flight/PlaneModel.cpp`
- Test: `Tools/PlaneModelTests/PlaneModelTests.cpp`

**Interfaces:**
- Consumes: `PureMath::FPureVector` de `Source/FutebolAviao/Flight/PureMath.h`.
- Produces: `EPlaneShape`, `EPlaneTint`, `EPlaneModel`, `FPlanePart`, `FPlaneCollisionSphere`, `FPlaneParts`, `FPlaneCollision`, `PlaneModel::GetParts(EPlaneModel)`, `PlaneModel::GetCollision(EPlaneModel)`.

- [ ] **Step 1: Escrever o header puro**

Criar `Source/FutebolAviao/Flight/PlaneModel.h`:

```cpp
#pragma once

#include "PureMath.h"

// Descricao dos modelos de aviao como DADO PURO -- sem nenhum header da
// Unreal, igual FlightPhysics e BallPhysics. Quem vira componente e material
// e o adapter (PlaneMeshBuilder), do lado da engine.
//
// TODAS as medidas aqui estao em DIAMETROS DE BOLA, nunca em centimetros. E
// isso que faz "o aviao e proporcional a bola" ser uma propriedade
// verificavel do dado, em vez de um comentario que envelhece: mudar o raio da
// bola reescala os tres avioes junto, e a suite trava a proporcao.
//
// Eixos locais: X pro nariz, Y pra asa direita, Z pra cima -- a mesma
// convencao que FFlightPhysicsState ja usa.

enum class EPlaneShape
{
	Cube,
	Cylinder,
	Cone,
	Sphere,
};

enum class EPlaneTint
{
	Team,    // recebe a cor do time
	Dark,    // fixa: nariz, helice, bocal
	Light,   // fixa: cabine, spinner
};

enum class EPlaneModel
{
	Delta,
	Warbird,
	Arcade,
};

struct FPlanePart
{
	EPlaneShape Shape = EPlaneShape::Cube;

	// Centro da peca, relativo ao centro do aviao, em diametros de bola.
	PureMath::FPureVector Offset;

	// Tamanho TOTAL da peca nos eixos do AVIAO (X = comprimento, Y = largura,
	// Z = altura), em diametros de bola.
	//
	// Nao e o tamanho no frame do mesh. Cilindro e cone de /Engine/BasicShapes
	// nascem com o eixo em Z, e deitar a peca na direcao do nariz e problema
	// do adapter: ele faz esse conserto sozinho a partir do Shape (ver
	// PartMeshScale e PartRotation, na Task 3). Se o conserto de eixo morasse
	// aqui, Size deixaria de significar comprimento e cada linha de tabela
	// poderia erra-lo por conta propria.
	PureMath::FPureVector Size;

	// Rotacao de DESIGN, em graus -- NAO inclui o conserto de eixo das
	// primitivas. Hoje so a asa enflechada do delta usa isto.
	float PitchDeg = 0.f;
	float YawDeg = 0.f;
	float RollDeg = 0.f;

	EPlaneTint Tint = EPlaneTint::Team;

	// Helice: gira em torno do X local a cada tick.
	bool bSpins = false;
};

struct FPlaneCollisionSphere
{
	PureMath::FPureVector Offset;   // em diametros de bola
	float Radius = 0.f;             // em diametros de bola
};

// Arrays de tamanho fixo devolvidos por valor: o codigo puro nao aloca e nao
// usa STL, pra continuar compilando tanto dentro da engine quanto nas suites
// standalone de Tools/.
struct FPlaneParts
{
	static constexpr int MaxParts = 16;
	FPlanePart Parts[MaxParts];
	int Count = 0;
};

struct FPlaneCollision
{
	static constexpr int MaxSpheres = 4;
	FPlaneCollisionSphere Spheres[MaxSpheres];
	int Count = 0;
};

namespace PlaneModel
{
	FPlaneParts GetParts(EPlaneModel Model);

	// Colisao contra a bola, em espaco local. Hoje TODO modelo devolve
	// exatamente uma esfera, centrada, de raio 0.75 -- comportamento
	// identico ao do cone que existia antes. A assinatura ja e plural de
	// proposito: dar a asa uma esfera separada da fuselagem depois vira
	// acrescentar linhas nesta tabela e um laco no ABallActor, sem mexer
	// em arquitetura.
	FPlaneCollision GetCollision(EPlaneModel Model);
}
```

- [ ] **Step 2: Escrever a suíte de testes (vai falhar por falta do .cpp)**

Criar `Tools/PlaneModelTests/PlaneModelTests.cpp`. Os helpers de medição são o coração da suíte: eles medem a **caixa envolvente depois da rotação**, porque a envergadura efetiva de uma peça girada não é o seu `Size.Y` — sem isso o delta passaria medindo a coisa errada.

```cpp
#include "../../Source/FutebolAviao/Flight/PlaneModel.h"
#include <cassert>
#include <cstdio>
#include <cmath>

// Proporcao alvo, em diametros de bola. E a regra da spec: o aviao mede 1.5
// diametro de bola em comprimento e 1.5 em envergadura.
static const float TargetLength = 1.5f;
static const float TargetSpan = 1.5f;

// O canto mais distante pode passar do raio de colisao ate 15%. NAO e "cabe
// dentro": se a ponta da cauda toca a esfera em Y=Z=0, qualquer leme com
// altura naquele ponto fica de fora -- nenhum aviao de verdade passaria num
// teste de encaixe perfeito. O que este limite pega e o caso que importa: uma
// asa desenhada muito maior que a colisao, atravessando a bola sem toca-la.
static const float MaxCornerOverRadius = 1.15f;

struct FBounds
{
	float MinX = 0.f, MaxX = 0.f;
	float MinY = 0.f, MaxY = 0.f;
	float FarthestCorner = 0.f;   // distancia do canto mais longe ate a origem
	bool bEmpty = true;
};

static void Accumulate(FBounds& Bounds, float X, float Y, float Z)
{
	if (Bounds.bEmpty)
	{
		Bounds.MinX = Bounds.MaxX = X;
		Bounds.MinY = Bounds.MaxY = Y;
		Bounds.bEmpty = false;
	}
	if (X < Bounds.MinX) Bounds.MinX = X;
	if (X > Bounds.MaxX) Bounds.MaxX = X;
	if (Y < Bounds.MinY) Bounds.MinY = Y;
	if (Y > Bounds.MaxY) Bounds.MaxY = Y;

	const float Distance = std::sqrt(X * X + Y * Y + Z * Z);
	if (Distance > Bounds.FarthestCorner) Bounds.FarthestCorner = Distance;
}

// Mede os 8 cantos de cada peca, aplicando o yaw de design. So yaw: e a unica
// rotacao de design que os modelos usam hoje. O assert de pitch/roll abaixo
// existe pra que este helper NUNCA meca errado em silencio -- se alguem
// acrescentar uma peca inclinada sem ensinar o helper a girar naquele eixo, o
// teste cai em vez de aprovar uma medida falsa.
static FBounds MeasureModel(EPlaneModel Model)
{
	const FPlaneParts Parts = PlaneModel::GetParts(Model);
	FBounds Bounds;

	for (int Index = 0; Index < Parts.Count; ++Index)
	{
		const FPlanePart& Part = Parts.Parts[Index];
		assert(Part.PitchDeg == 0.f && Part.RollDeg == 0.f);

		const float Radians = Part.YawDeg * 3.14159265f / 180.f;
		const float CosYaw = std::cos(Radians);
		const float SinYaw = std::sin(Radians);

		const float HalfX = Part.Size.X * 0.5f;
		const float HalfY = Part.Size.Y * 0.5f;
		const float HalfZ = Part.Size.Z * 0.5f;

		for (int Sx = -1; Sx <= 1; Sx += 2)
		{
			for (int Sy = -1; Sy <= 1; Sy += 2)
			{
				for (int Sz = -1; Sz <= 1; Sz += 2)
				{
					const float LocalX = Sx * HalfX;
					const float LocalY = Sy * HalfY;
					const float LocalZ = Sz * HalfZ;

					Accumulate(Bounds,
						LocalX * CosYaw - LocalY * SinYaw + Part.Offset.X,
						LocalX * SinYaw + LocalY * CosYaw + Part.Offset.Y,
						LocalZ + Part.Offset.Z);
				}
			}
		}
	}

	return Bounds;
}

static void Test_ArcadeIsOnePointFiveBallDiametersLong()
{
	const FBounds Bounds = MeasureModel(EPlaneModel::Arcade);
	const float Length = Bounds.MaxX - Bounds.MinX;

	assert(std::fabs(Length / TargetLength - 1.f) <= 0.03f);
	printf("Test_ArcadeIsOnePointFiveBallDiametersLong passed (%.3f)\n", Length);
}

static void Test_ArcadeIsOnePointFiveBallDiametersWide()
{
	const FBounds Bounds = MeasureModel(EPlaneModel::Arcade);
	const float Span = Bounds.MaxY - Bounds.MinY;

	assert(std::fabs(Span / TargetSpan - 1.f) <= 0.03f);
	printf("Test_ArcadeIsOnePointFiveBallDiametersWide passed (%.3f)\n", Span);
}

static void Test_ArcadeStaysCloseToItsCollisionSphere()
{
	const FPlaneCollision Collision = PlaneModel::GetCollision(EPlaneModel::Arcade);
	assert(Collision.Count >= 1);

	const FBounds Bounds = MeasureModel(EPlaneModel::Arcade);
	const float Ratio = Bounds.FarthestCorner / Collision.Spheres[0].Radius;

	assert(Ratio <= MaxCornerOverRadius);
	printf("Test_ArcadeStaysCloseToItsCollisionSphere passed (%.3f)\n", Ratio);
}

static void Test_ArcadeHasACollisionSphereWithPositiveRadius()
{
	const FPlaneCollision Collision = PlaneModel::GetCollision(EPlaneModel::Arcade);

	assert(Collision.Count >= 1);
	assert(Collision.Count <= FPlaneCollision::MaxSpheres);
	assert(Collision.Spheres[0].Radius > 0.f);
	printf("Test_ArcadeHasACollisionSphereWithPositiveRadius passed\n");
}

static void Test_ArcadeHasAtLeastOneTeamColoredPart()
{
	// Sem nenhuma peca Team, o aviao existe mas o time nao aparece -- a cor
	// nao teria onde pousar.
	const FPlaneParts Parts = PlaneModel::GetParts(EPlaneModel::Arcade);
	bool bFoundTeamPart = false;

	for (int Index = 0; Index < Parts.Count; ++Index)
	{
		if (Parts.Parts[Index].Tint == EPlaneTint::Team) bFoundTeamPart = true;
	}

	assert(bFoundTeamPart);
	printf("Test_ArcadeHasAtLeastOneTeamColoredPart passed\n");
}

static void Test_ArcadeFitsInTheFixedArrays()
{
	const FPlaneParts Parts = PlaneModel::GetParts(EPlaneModel::Arcade);

	assert(Parts.Count > 0);
	assert(Parts.Count <= FPlaneParts::MaxParts);
	printf("Test_ArcadeFitsInTheFixedArrays passed (%d pecas)\n", Parts.Count);
}

int main()
{
	Test_ArcadeIsOnePointFiveBallDiametersLong();
	Test_ArcadeIsOnePointFiveBallDiametersWide();
	Test_ArcadeStaysCloseToItsCollisionSphere();
	Test_ArcadeHasACollisionSphereWithPositiveRadius();
	Test_ArcadeHasAtLeastOneTeamColoredPart();
	Test_ArcadeFitsInTheFixedArrays();
	printf("All tests passed\n");
	return 0;
}
```

- [ ] **Step 3: Rodar os testes e confirmar que falham**

```bash
powershell -ExecutionPolicy Bypass -File Tools/run-tests.ps1
```

Esperado: **falha de compilação** da suíte `PlaneModel`, com erro de símbolo não resolvido para `PlaneModel::GetParts` e `PlaneModel::GetCollision` — o header existe, o `.cpp` ainda não. Ruído esperado e inofensivo: a linha `'vswhere.exe' is not recognized`.

- [ ] **Step 4: Escrever a tabela do modelo Arcade**

Criar `Source/FutebolAviao/Flight/PlaneModel.cpp`:

```cpp
#include "PlaneModel.h"

namespace
{
	PureMath::FPureVector MakeVector(float X, float Y, float Z)
	{
		PureMath::FPureVector Result;
		Result.X = X;
		Result.Y = Y;
		Result.Z = Z;
		return Result;
	}

	void AddPart(FPlaneParts& Parts, EPlaneShape Shape,
		float OffsetX, float OffsetY, float OffsetZ,
		float SizeX, float SizeY, float SizeZ,
		EPlaneTint Tint, float YawDeg = 0.f, bool bSpins = false)
	{
		if (Parts.Count >= FPlaneParts::MaxParts)
		{
			return;   // tabela cheia: a suite trava a contagem, ver Task 1
		}

		FPlanePart& Part = Parts.Parts[Parts.Count++];
		Part.Shape = Shape;
		Part.Offset = MakeVector(OffsetX, OffsetY, OffsetZ);
		Part.Size = MakeVector(SizeX, SizeY, SizeZ);
		Part.YawDeg = YawDeg;
		Part.Tint = Tint;
		Part.bSpins = bSpins;
	}

	FPlaneParts BuildArcade()
	{
		FPlaneParts Parts;
		//       forma                   offset x/y/z          size x/y/z            tint
		AddPart(Parts, EPlaneShape::Cylinder,  0.00f,  0.f,  0.00f,  1.10f, 0.42f, 0.42f, EPlaneTint::Team);
		AddPart(Parts, EPlaneShape::Cone,      0.62f,  0.f,  0.00f,  0.20f, 0.40f, 0.40f, EPlaneTint::Dark);
		AddPart(Parts, EPlaneShape::Sphere,    0.10f,  0.f,  0.22f,  0.34f, 0.30f, 0.24f, EPlaneTint::Light);
		AddPart(Parts, EPlaneShape::Cube,      0.02f,  0.f,  0.04f,  0.38f, 1.50f, 0.07f, EPlaneTint::Team);
		AddPart(Parts, EPlaneShape::Cube,     -0.58f,  0.f,  0.06f,  0.22f, 0.60f, 0.06f, EPlaneTint::Team);
		AddPart(Parts, EPlaneShape::Cube,     -0.60f,  0.f,  0.26f,  0.24f, 0.06f, 0.36f, EPlaneTint::Team);
		AddPart(Parts, EPlaneShape::Cube,      0.69f,  0.f,  0.00f,  0.03f, 0.06f, 0.56f, EPlaneTint::Dark, 0.f, true);
		AddPart(Parts, EPlaneShape::Sphere,    0.70f,  0.f,  0.00f,  0.10f, 0.10f, 0.10f, EPlaneTint::Light);
		return Parts;
	}
}

FPlaneParts PlaneModel::GetParts(EPlaneModel Model)
{
	switch (Model)
	{
	case EPlaneModel::Arcade:
	default:
		return BuildArcade();
	}
}

FPlaneCollision PlaneModel::GetCollision(EPlaneModel Model)
{
	// Uma esfera por modelo, centrada, de raio igual a METADE do comprimento
	// do aviao (1.5 / 2). Com a bola de hoje (raio 400, diametro 800) isso da
	// 600 -- exatamente o DefaultCollisionRadius que ja esta em PlanePawn.h.
	// O numero nao muda; ele passa a ter origem.
	(void)Model;

	FPlaneCollision Collision;
	Collision.Count = 1;
	Collision.Spheres[0].Radius = 0.75f;
	return Collision;
}
```

- [ ] **Step 5: Rodar os testes e confirmar que passam**

```bash
powershell -ExecutionPolicy Bypass -File Tools/run-tests.ps1
```

Esperado: as seis asserções do Arcade passam, e as quatro suítes que já existiam (`BallPhysics`, `FlightPhysics`, `FuelSystem`, `MatchRules`) continuam passando. Exit code 0.

- [ ] **Step 6: Commit**

```bash
git add Source/FutebolAviao/Flight/PlaneModel.h Source/FutebolAviao/Flight/PlaneModel.cpp Tools/PlaneModelTests/PlaneModelTests.cpp
git commit -m "feat: describe the arcade plane as pure data measured in ball diameters"
```

---

### Task 2: Os modelos Warbird e Delta

**Files:**
- Modify: `Source/FutebolAviao/Flight/PlaneModel.cpp`
- Test: `Tools/PlaneModelTests/PlaneModelTests.cpp`

**Interfaces:**
- Consumes: tudo o que a Task 1 produziu.
- Produces: `PlaneModel::GetParts` passa a responder para `EPlaneModel::Warbird` e `EPlaneModel::Delta`.

- [ ] **Step 1: Generalizar os testes para os três modelos**

Substituir os seis testes específicos do Arcade por versões que recebem o modelo, e chamar cada uma para os três. O delta é o que exercita o yaw: sem a rotação aplicada, a envergadura dele seria medida errada.

Trocar as funções de teste de `Tools/PlaneModelTests/PlaneModelTests.cpp` por:

```cpp
static const char* ModelName(EPlaneModel Model)
{
	switch (Model)
	{
	case EPlaneModel::Delta:   return "Delta";
	case EPlaneModel::Warbird: return "Warbird";
	case EPlaneModel::Arcade:  return "Arcade";
	}
	return "?";
}

static void Test_ModelIsOnePointFiveBallDiametersLong(EPlaneModel Model)
{
	const FBounds Bounds = MeasureModel(Model);
	const float Length = Bounds.MaxX - Bounds.MinX;

	assert(std::fabs(Length / TargetLength - 1.f) <= 0.03f);
	printf("Test_ModelIsOnePointFiveBallDiametersLong[%s] passed (%.3f)\n", ModelName(Model), Length);
}

static void Test_ModelIsOnePointFiveBallDiametersWide(EPlaneModel Model)
{
	const FBounds Bounds = MeasureModel(Model);
	const float Span = Bounds.MaxY - Bounds.MinY;

	assert(std::fabs(Span / TargetSpan - 1.f) <= 0.03f);
	printf("Test_ModelIsOnePointFiveBallDiametersWide[%s] passed (%.3f)\n", ModelName(Model), Span);
}

static void Test_ModelStaysCloseToItsCollisionSphere(EPlaneModel Model)
{
	const FPlaneCollision Collision = PlaneModel::GetCollision(Model);
	assert(Collision.Count >= 1);

	const FBounds Bounds = MeasureModel(Model);
	const float Ratio = Bounds.FarthestCorner / Collision.Spheres[0].Radius;

	assert(Ratio <= MaxCornerOverRadius);
	printf("Test_ModelStaysCloseToItsCollisionSphere[%s] passed (%.3f)\n", ModelName(Model), Ratio);
}

static void Test_ModelHasACollisionSphereWithPositiveRadius(EPlaneModel Model)
{
	const FPlaneCollision Collision = PlaneModel::GetCollision(Model);

	assert(Collision.Count >= 1);
	assert(Collision.Count <= FPlaneCollision::MaxSpheres);
	assert(Collision.Spheres[0].Radius > 0.f);
	printf("Test_ModelHasACollisionSphereWithPositiveRadius[%s] passed\n", ModelName(Model));
}

static void Test_ModelHasAtLeastOneTeamColoredPart(EPlaneModel Model)
{
	const FPlaneParts Parts = PlaneModel::GetParts(Model);
	bool bFoundTeamPart = false;

	for (int Index = 0; Index < Parts.Count; ++Index)
	{
		if (Parts.Parts[Index].Tint == EPlaneTint::Team) bFoundTeamPart = true;
	}

	assert(bFoundTeamPart);
	printf("Test_ModelHasAtLeastOneTeamColoredPart[%s] passed\n", ModelName(Model));
}

static void Test_ModelFitsInTheFixedArrays(EPlaneModel Model)
{
	const FPlaneParts Parts = PlaneModel::GetParts(Model);

	assert(Parts.Count > 0);
	assert(Parts.Count <= FPlaneParts::MaxParts);
	printf("Test_ModelFitsInTheFixedArrays[%s] passed (%d pecas)\n", ModelName(Model), Parts.Count);
}

// Cilindro e cone nascem com o eixo em Z e o adapter conserta esse eixo com um
// pitch de 90 graus (Task 3). Compor esse conserto com uma rotacao de design
// nao e soma de angulos, e a Task 3 nao tenta fazer essa composicao. Enquanto
// nao fizer, uma peca redonda com rotacao de design sairia torta em silencio;
// este teste torna isso impossivel de passar despercebido.
static void Test_RoundPartsHaveNoDesignRotation(EPlaneModel Model)
{
	const FPlaneParts Parts = PlaneModel::GetParts(Model);

	for (int Index = 0; Index < Parts.Count; ++Index)
	{
		const FPlanePart& Part = Parts.Parts[Index];
		const bool bIsRound = Part.Shape == EPlaneShape::Cylinder || Part.Shape == EPlaneShape::Cone;
		if (bIsRound)
		{
			assert(Part.PitchDeg == 0.f && Part.YawDeg == 0.f && Part.RollDeg == 0.f);
		}
	}

	printf("Test_RoundPartsHaveNoDesignRotation[%s] passed\n", ModelName(Model));
}

// Sem isto, a Task 2 nao teria um teste capaz de falhar: enquanto GetParts
// cai no default e devolve o arcade pros tres, todas as assercoes acima
// passam -- medindo o arcade tres vezes e aprovando dois modelos que nao
// existem. Este teste e o que exige que os tres sejam realmente distintos.
static void Test_TheThreeModelsAreDifferentFromEachOther()
{
	const FPlaneParts Delta = PlaneModel::GetParts(EPlaneModel::Delta);
	const FPlaneParts Warbird = PlaneModel::GetParts(EPlaneModel::Warbird);
	const FPlaneParts Arcade = PlaneModel::GetParts(EPlaneModel::Arcade);

	// O delta e o unico com asa enflechada; o warbird e o arcade sao os unicos
	// com helice. Duas propriedades que separam os tres dois a dois.
	bool bDeltaHasSweptWing = false;
	bool bDeltaHasPropeller = false;
	for (int Index = 0; Index < Delta.Count; ++Index)
	{
		if (Delta.Parts[Index].YawDeg != 0.f) bDeltaHasSweptWing = true;
		if (Delta.Parts[Index].bSpins) bDeltaHasPropeller = true;
	}

	bool bWarbirdHasPropeller = false;
	for (int Index = 0; Index < Warbird.Count; ++Index)
	{
		if (Warbird.Parts[Index].bSpins) bWarbirdHasPropeller = true;
	}

	bool bArcadeHasSweptWing = false;
	for (int Index = 0; Index < Arcade.Count; ++Index)
	{
		if (Arcade.Parts[Index].YawDeg != 0.f) bArcadeHasSweptWing = true;
	}

	assert(bDeltaHasSweptWing);
	assert(!bDeltaHasPropeller);
	assert(bWarbirdHasPropeller);
	assert(!bArcadeHasSweptWing);
	assert(Delta.Count != Warbird.Count);
	printf("Test_TheThreeModelsAreDifferentFromEachOther passed\n");
}

int main()
{
	const EPlaneModel Models[] = { EPlaneModel::Delta, EPlaneModel::Warbird, EPlaneModel::Arcade };

	Test_TheThreeModelsAreDifferentFromEachOther();

	for (const EPlaneModel Model : Models)
	{
		Test_ModelIsOnePointFiveBallDiametersLong(Model);
		Test_ModelIsOnePointFiveBallDiametersWide(Model);
		Test_ModelStaysCloseToItsCollisionSphere(Model);
		Test_ModelHasACollisionSphereWithPositiveRadius(Model);
		Test_ModelHasAtLeastOneTeamColoredPart(Model);
		Test_ModelFitsInTheFixedArrays(Model);
		Test_RoundPartsHaveNoDesignRotation(Model);
	}

	printf("All tests passed\n");
	return 0;
}
```

- [ ] **Step 2: Rodar os testes e confirmar que falham**

```bash
powershell -ExecutionPolicy Bypass -File Tools/run-tests.ps1
```

Esperado: **falha por assert em `Test_TheThreeModelsAreDifferentFromEachOther`**, na linha `assert(bDeltaHasSweptWing)`. `GetParts` ainda cai no `default` e devolve o arcade para os três, e o arcade não tem asa enflechada.

Os testes de comprimento e envergadura **passam** neste ponto, e isso é esperado: eles estariam medindo o arcade três vezes. É exatamente por isso que este teste existe — sem ele a Task 2 não teria como falhar antes de ser implementada, e "verde" não significaria nada.

- [ ] **Step 3: Escrever as duas tabelas**

Em `Source/FutebolAviao/Flight/PlaneModel.cpp`, dentro do `namespace` anônimo, acrescentar depois de `BuildArcade`:

```cpp
	FPlaneParts BuildWarbird()
	{
		FPlaneParts Parts;
		//       forma                   offset x/y/z          size x/y/z            tint
		AddPart(Parts, EPlaneShape::Cylinder,  0.00f,  0.f,  0.00f,  1.15f, 0.32f, 0.32f, EPlaneTint::Team);
		AddPart(Parts, EPlaneShape::Cylinder,  0.62f,  0.f,  0.00f,  0.20f, 0.34f, 0.34f, EPlaneTint::Team);
		AddPart(Parts, EPlaneShape::Cube,      0.70f,  0.f,  0.00f,  0.03f, 0.06f, 0.48f, EPlaneTint::Dark, 0.f, true);
		AddPart(Parts, EPlaneShape::Cone,      0.71f,  0.f,  0.00f,  0.06f, 0.12f, 0.12f, EPlaneTint::Light);
		AddPart(Parts, EPlaneShape::Sphere,    0.05f,  0.f,  0.18f,  0.32f, 0.24f, 0.18f, EPlaneTint::Light);
		AddPart(Parts, EPlaneShape::Cube,      0.05f,  0.f, -0.02f,  0.34f, 1.50f, 0.05f, EPlaneTint::Team);
		AddPart(Parts, EPlaneShape::Cube,     -0.55f,  0.f,  0.02f,  0.20f, 0.52f, 0.04f, EPlaneTint::Team);
		AddPart(Parts, EPlaneShape::Cube,     -0.60f,  0.f,  0.22f,  0.26f, 0.05f, 0.32f, EPlaneTint::Team);
		return Parts;
	}

	FPlaneParts BuildDelta()
	{
		FPlaneParts Parts;
		//       forma                   offset x/y/z          size x/y/z            tint                     yaw
		AddPart(Parts, EPlaneShape::Cylinder,  0.00f,  0.00f,  0.00f,  1.30f, 0.30f, 0.30f, EPlaneTint::Team);
		AddPart(Parts, EPlaneShape::Cone,      0.68f,  0.00f,  0.00f,  0.16f, 0.28f, 0.28f, EPlaneTint::Dark);
		AddPart(Parts, EPlaneShape::Sphere,    0.28f,  0.00f,  0.16f,  0.30f, 0.22f, 0.18f, EPlaneTint::Light);
		AddPart(Parts, EPlaneShape::Cube,     -0.10f, -0.33f,  0.00f,  0.50f, 0.70f, 0.05f, EPlaneTint::Team, -18.f);
		AddPart(Parts, EPlaneShape::Cube,     -0.10f,  0.33f,  0.00f,  0.50f, 0.70f, 0.05f, EPlaneTint::Team,  18.f);
		AddPart(Parts, EPlaneShape::Cube,     -0.55f,  0.00f,  0.26f,  0.30f, 0.05f, 0.38f, EPlaneTint::Team);
		AddPart(Parts, EPlaneShape::Cylinder, -0.68f,  0.00f,  0.00f,  0.12f, 0.26f, 0.26f, EPlaneTint::Dark);
		return Parts;
	}
```

E trocar o `switch` de `GetParts` por:

```cpp
FPlaneParts PlaneModel::GetParts(EPlaneModel Model)
{
	switch (Model)
	{
	case EPlaneModel::Delta:
		return BuildDelta();
	case EPlaneModel::Warbird:
		return BuildWarbird();
	case EPlaneModel::Arcade:
	default:
		return BuildArcade();
	}
}
```

- [ ] **Step 4: Rodar os testes e confirmar que passam**

```bash
powershell -ExecutionPolicy Bypass -File Tools/run-tests.ps1
```

Esperado: 22 linhas de `passed` (7 testes × 3 modelos, mais `Test_TheThreeModelsAreDifferentFromEachOther`) e as quatro suítes antigas verdes. Os valores impressos devem bater com os da spec: comprimento 1.500 / 1.470 / 1.470 e envergadura 1.480 / 1.500 / 1.500 para Delta / Warbird / Arcade.

- [ ] **Step 5: Commit**

```bash
git add Source/FutebolAviao/Flight/PlaneModel.cpp Tools/PlaneModelTests/PlaneModelTests.cpp
git commit -m "feat: add the warbird and delta plane models"
```

---

### Task 3: Conversão de diâmetros de bola para centímetros

**Files:**
- Modify: `Source/FutebolAviao/Flight/PlaneModel.h`
- Modify: `Source/FutebolAviao/Flight/PlaneModel.cpp`
- Test: `Tools/PlaneModelTests/PlaneModelTests.cpp`

**Interfaces:**
- Consumes: `FPlanePart`, `FPlaneCollision` da Task 1.
- Produces: `PlaneModel::PartLocation(const FPlanePart&, float BallDiameter) -> PureMath::FPureVector`, `PlaneModel::PartMeshScale(const FPlanePart&, float BallDiameter) -> PureMath::FPureVector`, `PlaneModel::PartPitchDeg(const FPlanePart&) -> float`, `PlaneModel::CollisionRadiusFor(EPlaneModel, float BallDiameter) -> float`.

Esta é a fronteira de unidades: o único lugar do sistema que sabe que existe centímetro. Fica no lado puro justamente para ser testável — a Task 4, do lado da Unreal, não tem suíte.

- [ ] **Step 1: Escrever os testes de conversão**

Acrescentar a `Tools/PlaneModelTests/PlaneModelTests.cpp`, antes do `main`:

```cpp
static bool NearlyEqual(float A, float B, float Tolerance = 0.01f)
{
	return std::fabs(A - B) <= Tolerance;
}

// A afirmacao de manchete da spec, travada por teste: com a bola de hoje
// (raio 400, diametro 800), o raio de colisao derivado do modelo tem que dar
// exatamente o 600 que ja esta em PlanePawn::DefaultCollisionRadius. Se este
// teste cair, ou a proporcao mudou ou o numero do pawn ficou orfao.
static void Test_CollisionRadiusMatchesTheValueAlreadyInThePawn()
{
	assert(NearlyEqual(PlaneModel::CollisionRadiusFor(EPlaneModel::Arcade, 800.f), 600.f));
	assert(NearlyEqual(PlaneModel::CollisionRadiusFor(EPlaneModel::Warbird, 800.f), 600.f));
	assert(NearlyEqual(PlaneModel::CollisionRadiusFor(EPlaneModel::Delta, 800.f), 600.f));
	printf("Test_CollisionRadiusMatchesTheValueAlreadyInThePawn passed\n");
}

static void Test_ThePlaneGrowsWithTheBall()
{
	// A proporcao e a regra; o tamanho e consequencia. Bola do dobro, aviao do
	// dobro -- e isso que faz mexer em fa.Ball.Radius durante o Play nao
	// quebrar a proporcao.
	const float Small = PlaneModel::CollisionRadiusFor(EPlaneModel::Arcade, 800.f);
	const float Large = PlaneModel::CollisionRadiusFor(EPlaneModel::Arcade, 1600.f);

	assert(NearlyEqual(Large, Small * 2.f));
	printf("Test_ThePlaneGrowsWithTheBall passed\n");
}

static void Test_PartLocationScalesTheOffsetByTheBallDiameter()
{
	FPlanePart Part;
	Part.Offset.X = 0.5f;
	Part.Offset.Z = -0.25f;

	const PureMath::FPureVector Location = PlaneModel::PartLocation(Part, 800.f);

	assert(NearlyEqual(Location.X, 400.f));
	assert(NearlyEqual(Location.Y, 0.f));
	assert(NearlyEqual(Location.Z, -200.f));
	printf("Test_PartLocationScalesTheOffsetByTheBallDiameter passed\n");
}

static void Test_BoxPartScalesStraightFromItsSize()
{
	// Cubo e esfera de /Engine/BasicShapes tem 100 de lado, e o eixo deles ja
	// e o do aviao: a escala e o tamanho dividido por 100, sem troca de eixo.
	FPlanePart Part;
	Part.Shape = EPlaneShape::Cube;
	Part.Size.X = 1.f;
	Part.Size.Y = 0.5f;
	Part.Size.Z = 0.25f;

	const PureMath::FPureVector Scale = PlaneModel::PartMeshScale(Part, 800.f);

	assert(NearlyEqual(Scale.X, 8.f));
	assert(NearlyEqual(Scale.Y, 4.f));
	assert(NearlyEqual(Scale.Z, 2.f));
	printf("Test_BoxPartScalesStraightFromItsSize passed\n");
}

static void Test_RoundPartSwapsAxesBecauseItIsBornAlongZ()
{
	// Cilindro e cone nascem com o eixo em Z. O adapter os deita com pitch 90,
	// o que manda o Z do mesh pro X do aviao e o X do mesh pro Z. Logo o
	// comprimento pedido (Size.X) tem que sair na escala Z do mesh, e a altura
	// (Size.Z) na escala X -- senao uma fuselagem de 1.10 de comprimento sai
	// com 1.10 de ALTURA, em pe no lugar de deitada.
	FPlanePart Part;
	Part.Shape = EPlaneShape::Cylinder;
	Part.Size.X = 1.f;      // comprimento
	Part.Size.Y = 0.5f;     // largura
	Part.Size.Z = 0.25f;    // altura

	const PureMath::FPureVector Scale = PlaneModel::PartMeshScale(Part, 800.f);

	assert(NearlyEqual(Scale.X, 2.f));   // altura
	assert(NearlyEqual(Scale.Y, 4.f));   // largura
	assert(NearlyEqual(Scale.Z, 8.f));   // comprimento
	assert(NearlyEqual(PlaneModel::PartPitchDeg(Part), 90.f));
	printf("Test_RoundPartSwapsAxesBecauseItIsBornAlongZ passed\n");
}

static void Test_BoxPartIsNotPitchedByTheAxisFixUp()
{
	FPlanePart Part;
	Part.Shape = EPlaneShape::Cube;
	Part.PitchDeg = 0.f;

	assert(NearlyEqual(PlaneModel::PartPitchDeg(Part), 0.f));
	printf("Test_BoxPartIsNotPitchedByTheAxisFixUp passed\n");
}
```

E chamá-los no `main`, depois do laço dos modelos:

```cpp
	Test_CollisionRadiusMatchesTheValueAlreadyInThePawn();
	Test_ThePlaneGrowsWithTheBall();
	Test_PartLocationScalesTheOffsetByTheBallDiameter();
	Test_BoxPartScalesStraightFromItsSize();
	Test_RoundPartSwapsAxesBecauseItIsBornAlongZ();
	Test_BoxPartIsNotPitchedByTheAxisFixUp();
```

- [ ] **Step 2: Rodar os testes e confirmar que falham**

```bash
powershell -ExecutionPolicy Bypass -File Tools/run-tests.ps1
```

Esperado: falha de compilação, `PlaneModel::CollisionRadiusFor` e as outras três funções não declaradas.

- [ ] **Step 3: Declarar as funções no header**

Acrescentar dentro do `namespace PlaneModel`, em `Source/FutebolAviao/Flight/PlaneModel.h`:

```cpp
	// A FRONTEIRA DE UNIDADES. Tudo acima desta linha e medido em diametros de
	// bola; estas quatro funcoes sao o unico lugar do sistema que sabe que
	// centimetro existe. Ficam no lado puro de proposito: o adapter da Unreal
	// nao tem suite standalone, e sem isto a conversao seria a unica parte
	// nao testada da cadeia toda.

	// Posicao da peca em centimetros, relativa ao centro do aviao.
	PureMath::FPureVector PartLocation(const FPlanePart& Part, float BallDiameter);

	// Escala a aplicar no mesh. Todas as primitivas de /Engine/BasicShapes tem
	// 100 de lado. Cilindro e cone trocam de eixo -- ver PartPitchDeg.
	PureMath::FPureVector PartMeshScale(const FPlanePart& Part, float BallDiameter);

	// Pitch final da peca: a rotacao de design mais o conserto de eixo das
	// primitivas redondas (90 graus, que deita o eixo Z do mesh na direcao do
	// nariz). Cilindro e cone nao podem ter rotacao de design junto -- compor
	// as duas nao e somar angulos, e a suite trava isso.
	float PartPitchDeg(const FPlanePart& Part);

	// Raio da esfera de colisao em centimetros. Com BallDiameter = 800 devolve
	// 600, o valor que ja esta em PlanePawn::DefaultCollisionRadius.
	float CollisionRadiusFor(EPlaneModel Model, float BallDiameter);
```

- [ ] **Step 4: Implementar as conversões**

Acrescentar ao fim de `Source/FutebolAviao/Flight/PlaneModel.cpp`:

```cpp
namespace
{
	// Toda primitiva de /Engine/BasicShapes tem 100 de lado.
	const float BasicShapeSize = 100.f;

	bool IsRound(EPlaneShape Shape)
	{
		return Shape == EPlaneShape::Cylinder || Shape == EPlaneShape::Cone;
	}
}

PureMath::FPureVector PlaneModel::PartLocation(const FPlanePart& Part, float BallDiameter)
{
	return PureMath::Scale(Part.Offset, BallDiameter);
}

PureMath::FPureVector PlaneModel::PartMeshScale(const FPlanePart& Part, float BallDiameter)
{
	const float Factor = BallDiameter / BasicShapeSize;

	if (IsRound(Part.Shape))
	{
		// Pitch de 90 manda o Z do mesh pro X do aviao e o X do mesh pro Z.
		// A escala acompanha a troca, senao o comprimento pedido vira altura.
		return MakeVector(Part.Size.Z * Factor, Part.Size.Y * Factor, Part.Size.X * Factor);
	}

	return PureMath::Scale(Part.Size, Factor);
}

float PlaneModel::PartPitchDeg(const FPlanePart& Part)
{
	return IsRound(Part.Shape) ? Part.PitchDeg + 90.f : Part.PitchDeg;
}

float PlaneModel::CollisionRadiusFor(EPlaneModel Model, float BallDiameter)
{
	const FPlaneCollision Collision = GetCollision(Model);
	if (Collision.Count <= 0)
	{
		return 0.f;
	}

	return Collision.Spheres[0].Radius * BallDiameter;
}
```

Atenção: `MakeVector` está no `namespace` anônimo declarado no topo do arquivo, na Task 1. Se o novo `namespace` anônimo for adicionado depois das funções que o usam, o compilador não o enxerga — os dois blocos anônimos se fundem, mas a ordem de declaração continua valendo. Coloque `BasicShapeSize` e `IsRound` no bloco anônimo **já existente**, no topo, em vez de abrir um segundo.

- [ ] **Step 5: Rodar os testes e confirmar que passam**

```bash
powershell -ExecutionPolicy Bypass -File Tools/run-tests.ps1
```

Esperado: os 6 testes de conversão passam junto com os 21 dos modelos, e as quatro suítes antigas continuam verdes.

- [ ] **Step 6: Commit**

```bash
git add Source/FutebolAviao/Flight/PlaneModel.h Source/FutebolAviao/Flight/PlaneModel.cpp Tools/PlaneModelTests/PlaneModelTests.cpp
git commit -m "feat: convert plane model measurements from ball diameters to centimeters"
```

---

### Task 4: Montar o modelo no pawn e separar visual de colisão

**Files:**
- Create: `Source/FutebolAviao/Flight/PlaneMeshBuilder.h`
- Create: `Source/FutebolAviao/Flight/PlaneMeshBuilder.cpp`
- Modify: `Source/FutebolAviao/Flight/PlanePawn.h`
- Modify: `Source/FutebolAviao/Flight/PlanePawn.cpp`

**Interfaces:**
- Consumes: `PlaneModel::GetParts`, `PlaneModel::PartLocation`, `PlaneModel::PartMeshScale`, `PlaneModel::PartPitchDeg` das Tasks 1-3.
- Produces: `FPlaneMeshBuilder::Build(AActor& Owner, USceneComponent& Attach, EPlaneModel Model, float BallDiameter, TArray<UStaticMeshComponent*>& OutParts, TArray<UStaticMeshComponent*>& OutSpinningParts)` e `FPlaneMeshBuilder::Clear(TArray<UStaticMeshComponent*>& Parts, TArray<UStaticMeshComponent*>& SpinningParts)`.

**Esta task não tem teste automatizado.** As suítes de `Tools/` compilam sem a engine e não alcançam `UStaticMeshComponent`. A verificação é compilar e olhar. Isso é aceitável exatamente porque as Tasks 1-3 puxaram toda a aritmética para o lado puro: o que sobra aqui é criação de componente, sem cálculo.

- [ ] **Step 1: Escrever o builder**

Criar `Source/FutebolAviao/Flight/PlaneMeshBuilder.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "PlaneModel.h"

class AActor;
class USceneComponent;
class UStaticMeshComponent;

// Adapter que transforma a descricao pura de PlaneModel em componentes de
// verdade. Mora do lado da engine: PlaneModel nao pode incluir header da
// Unreal, e APlanePawn nao deveria crescer mais 150 linhas de montagem de
// malha em cima do que ja cuida (voo, input, combustivel, tuning).
//
// TODAS as pecas criadas aqui sao SEM COLISAO, de proposito. O visual do
// aviao nao participa de nenhuma fisica -- quem colide e a USphereComponent
// do pawn. E isso que faz trocar de modelo, engordar uma asa ou acrescentar
// uma peca nunca mudar como o aviao bate em nada.
struct FPlaneMeshBuilder
{
	static void Build(AActor& Owner, USceneComponent& Attach, EPlaneModel Model, float BallDiameter,
		TArray<UStaticMeshComponent*>& OutParts, TArray<UStaticMeshComponent*>& OutSpinningParts);

	static void Clear(TArray<UStaticMeshComponent*>& Parts, TArray<UStaticMeshComponent*>& SpinningParts);
};
```

Criar `Source/FutebolAviao/Flight/PlaneMeshBuilder.cpp`:

```cpp
#include "PlaneMeshBuilder.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* MeshPathFor(EPlaneShape Shape)
	{
		switch (Shape)
		{
		case EPlaneShape::Cylinder: return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		case EPlaneShape::Cone:     return TEXT("/Engine/BasicShapes/Cone.Cone");
		case EPlaneShape::Sphere:   return TEXT("/Engine/BasicShapes/Sphere.Sphere");
		case EPlaneShape::Cube:
		default:                    return TEXT("/Engine/BasicShapes/Cube.Cube");
		}
	}

	FVector ToUnreal(const PureMath::FPureVector& V)
	{
		return FVector(V.X, V.Y, V.Z);
	}
}

void FPlaneMeshBuilder::Build(AActor& Owner, USceneComponent& Attach, EPlaneModel Model, float BallDiameter,
	TArray<UStaticMeshComponent*>& OutParts, TArray<UStaticMeshComponent*>& OutSpinningParts)
{
	Clear(OutParts, OutSpinningParts);

	const FPlaneParts Parts = PlaneModel::GetParts(Model);

	for (int32 Index = 0; Index < Parts.Count; ++Index)
	{
		const FPlanePart& Part = Parts.Parts[Index];

		// NewObject em vez de CreateDefaultSubobject: o numero de pecas muda
		// com o modelo, e subobjeto default so pode nascer no construtor, com
		// nome fixo. Criar em runtime e o que permite trocar de aviao durante
		// o Play (Task 7).
		UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(&Owner);
		if (!Component)
		{
			continue;
		}

		Component->SetupAttachment(&Attach);
		Component->RegisterComponent();

		if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPathFor(Part.Shape)))
		{
			Component->SetStaticMesh(Mesh);
		}

		Component->SetRelativeLocation(ToUnreal(PlaneModel::PartLocation(Part, BallDiameter)));
		Component->SetRelativeScale3D(ToUnreal(PlaneModel::PartMeshScale(Part, BallDiameter)));
		Component->SetRelativeRotation(FRotator(PlaneModel::PartPitchDeg(Part), Part.YawDeg, Part.RollDeg));

		// Cosmetico e so cosmetico.
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);

		OutParts.Add(Component);
		if (Part.bSpins)
		{
			OutSpinningParts.Add(Component);
		}
	}
}

void FPlaneMeshBuilder::Clear(TArray<UStaticMeshComponent*>& Parts, TArray<UStaticMeshComponent*>& SpinningParts)
{
	for (UStaticMeshComponent* Component : Parts)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}

	Parts.Reset();
	SpinningParts.Reset();   // sao os MESMOS componentes, ja destruidos acima
}
```

- [ ] **Step 2: Trocar o cone pela esfera de colisão no pawn**

Em `Source/FutebolAviao/Flight/PlanePawn.h`, trocar a declaração de `MeshComponent` e acrescentar os campos novos. Substituir:

```cpp
	UPROPERTY(VisibleAnywhere, Category = "Plane")
	UStaticMeshComponent* MeshComponent;
```

por:

```cpp
	// A UNICA colisao do aviao. As pecas visuais nao colidem com nada -- ver
	// PlaneMeshBuilder. Separar as duas coisas e o que faz mudar o desenho do
	// aviao nunca mudar a fisica dele.
	UPROPERTY(VisibleAnywhere, Category = "Plane")
	USphereComponent* CollisionComponent;

	UPROPERTY()
	TArray<UStaticMeshComponent*> PartComponents;

	UPROPERTY()
	TArray<UStaticMeshComponent*> SpinningPartComponents;
```

No topo do mesmo header, trocar a forward declaration `class UStaticMeshComponent;` por:

```cpp
class UStaticMeshComponent;
class USphereComponent;
```

- [ ] **Step 3: Montar as peças no BeginPlay**

Em `Source/FutebolAviao/Flight/PlanePawn.cpp`, trocar os includes do topo:

```cpp
#include "PlanePawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "PlaneMeshBuilder.h"
#include "BallPhysics.h"
#include "../Tuning/TuningCVars.h"
```

`UObject/ConstructorHelpers.h` sai: não há mais `FObjectFinder`, o builder carrega as malhas com `LoadObject`.

Substituir o bloco do cone no construtor:

```cpp
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMeshAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(ConeMeshAsset.Object);
		MeshComponent->SetRelativeScale3D(FVector(12.f, 6.f, 6.f));
		MeshComponent->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	}
```

por:

```cpp
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->SetSphereRadius(DefaultCollisionRadius);
```

E acrescentar, no fim de `APlanePawn::BeginPlay`, depois do `ResetFlightStateTo(SpawnTransform)`:

```cpp
	// O tamanho do aviao sai do tamanho da bola: a proporcao e a regra, o
	// numero de centimetros e consequencia. Ver PlaneModel.h.
	const float BallDiameter = FBallPhysicsParams().Radius * 2.f;
	FPlaneMeshBuilder::Build(*this, *CollisionComponent, EPlaneModel::Arcade, BallDiameter,
		PartComponents, SpinningPartComponents);
```

- [ ] **Step 4: Compilar o editor**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FutebolAviaoEditor Win64 Development -Project="C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -WaitMutex
```

Esperado: build com sucesso. Se falhar em `USphereComponent`, confira o include `Components/SphereComponent.h`.

- [ ] **Step 5: Rodar as suítes puras para confirmar que nada quebrou**

```bash
powershell -ExecutionPolicy Bypass -File Tools/run-tests.ps1
```

Esperado: as cinco suítes verdes. Esta task não mexeu em código puro, então o resultado tem que ser idêntico ao da Task 3.

- [ ] **Step 6: Olhar o avião**

Abrir o editor, dar Play em `Content/Maps/TestFlightMap`, e tirar uma captura do avião em voo.

O que confere:
- O avião parece um avião, com nariz, asa e cauda distinguíveis.
- O **nariz aponta para a direção em que ele voa**. Se estiver apontando para cima ou para baixo, é o sinal do pitch do conserto de eixo: trocar `+ 90.f` por `- 90.f` em `PlaneModel::PartPitchDeg` e ajustar `Test_RoundPartSwapsAxesBecauseItIsBornAlongZ`. É o único risco de sinal do plano, e ele está isolado numa função só.
- O tamanho é comparável ao da bola: o avião mede uma vez e meia a bola.
- Bater numa parede da arena ainda para o avião (a esfera de colisão está funcionando).

**Mandar a captura para o usuário antes de seguir.** As tabelas de peças são chute informado; é aqui que elas encontram a realidade pela primeira vez.

- [ ] **Step 7: Commit**

```bash
git add Source/FutebolAviao/Flight/PlaneMeshBuilder.h Source/FutebolAviao/Flight/PlaneMeshBuilder.cpp Source/FutebolAviao/Flight/PlanePawn.h Source/FutebolAviao/Flight/PlanePawn.cpp
git commit -m "feat: build the plane out of primitives and give it a collision sphere of its own"
```

---

### Task 5: Cor de time

**Files:**
- Modify: `Source/FutebolAviao/Flight/PlaneMeshBuilder.h`
- Modify: `Source/FutebolAviao/Flight/PlaneMeshBuilder.cpp`
- Modify: `Source/FutebolAviao/Flight/PlanePawn.h`
- Modify: `Source/FutebolAviao/Flight/PlanePawn.cpp`

**Interfaces:**
- Consumes: `FPlaneMeshBuilder::Build` da Task 4, `EPlaneTint` da Task 1.
- Produces: `APlanePawn::SetTeamColor(FLinearColor)`, CVar `fa.Plane.Team`.

- [ ] **Step 1: Guardar as peças de time no builder**

Em `PlaneMeshBuilder.h`, acrescentar um quarto parâmetro de saída em `Build` e um método novo:

```cpp
	static void Build(AActor& Owner, USceneComponent& Attach, EPlaneModel Model, float BallDiameter,
		TArray<UStaticMeshComponent*>& OutParts, TArray<UStaticMeshComponent*>& OutSpinningParts,
		TArray<UMaterialInstanceDynamic*>& OutTeamMaterials);

	static void Clear(TArray<UStaticMeshComponent*>& Parts, TArray<UStaticMeshComponent*>& SpinningParts,
		TArray<UMaterialInstanceDynamic*>& TeamMaterials);
```

E a forward declaration `class UMaterialInstanceDynamic;` junto das outras.

Acrescentar também, fora do struct, o nome do parâmetro de cor:

```cpp
// Nome do VectorParameter de cor do BasicShapeMaterial. Mora num lugar so
// porque DOIS arquivos escrevem nele: o builder, ao montar a peca, e o pawn,
// quando a cor do time muda. Repetir o literal nos dois e a mesma classe de
// erro que produziu o bug do gol inalcancavel na Fase 3 -- um lado muda, o
// outro nao, e nada avisa.
extern const TCHAR* const PlaneColorParameterName;
```

E defini-lo em `PlaneMeshBuilder.cpp`, fora do `namespace` anônimo:

```cpp
const TCHAR* const PlaneColorParameterName = TEXT("Color");
```

- [ ] **Step 2: Criar os material instances na montagem**

Em `PlaneMeshBuilder.cpp`, acrescentar aos includes:

```cpp
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
```

E no `namespace` anônimo:

```cpp
	// O material das primitivas da engine expoe um VectorParameter chamado
	// "Color" (conferido inspecionando o .uasset). Se a criacao do instance
	// falhar, a peca fica com o material padrao -- cinza, nao quebrada.
	const TCHAR* BasicShapeMaterialPath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");

	FLinearColor ColorFor(EPlaneTint Tint)
	{
		switch (Tint)
		{
		case EPlaneTint::Dark:  return FLinearColor(0.05f, 0.05f, 0.06f);
		case EPlaneTint::Light: return FLinearColor(0.75f, 0.80f, 0.85f);
		case EPlaneTint::Team:
		default:                return FLinearColor::White;   // sobrescrita por SetTeamColor
		}
	}
```

Dentro do laço de `Build`, depois de `SetGenerateOverlapEvents(false)`:

```cpp
		if (UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, BasicShapeMaterialPath))
		{
			if (UMaterialInstanceDynamic* Material = Component->CreateDynamicMaterialInstance(0, BaseMaterial))
			{
				Material->SetVectorParameterValue(PlaneColorParameterName, ColorFor(Part.Tint));

				// So as pecas Team entram na lista: pintar o aviao inteiro de
				// uma cor chapada apaga a silhueta a 200m, que e justamente o
				// que a cor deveria estar ajudando a ler.
				if (Part.Tint == EPlaneTint::Team)
				{
					OutTeamMaterials.Add(Material);
				}
			}
		}
```

E em `Clear`, acrescentar `TeamMaterials.Reset();` (os materiais morrem com os componentes).

- [ ] **Step 3: Expor `SetTeamColor` no pawn**

Em `PlanePawn.h`, na seção pública, depois de `SetSpawnTransform`:

```cpp
	// Gancho pra Fase 4 escolher a cor de cada time. Hoje quem chama e a CVar
	// fa.Plane.Team, so pra dar pra testar.
	UFUNCTION(BlueprintCallable, Category = "Plane")
	void SetTeamColor(FLinearColor NewColor);
```

E nos campos privados:

```cpp
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> TeamMaterials;

	FLinearColor TeamColor = FLinearColor(0.15f, 0.35f, 0.85f);   // azul: Oeste
```

Com a forward declaration `class UMaterialInstanceDynamic;`.

- [ ] **Step 4: Implementar e ligar na CVar**

Em `PlanePawn.cpp`, acrescentar ao `namespace` anônimo das CVars:

```cpp
	TAutoConsoleVariable<int32> CVarPlaneTeam(TEXT("fa.Plane.Team"), 0, TEXT("Cor do time: 0 = azul (Oeste), 1 = vermelho (Leste)."));
```

Acrescentar o include `#include "Materials/MaterialInstanceDynamic.h"`.

A implementação:

```cpp
void APlanePawn::SetTeamColor(FLinearColor NewColor)
{
	TeamColor = NewColor;

	for (UMaterialInstanceDynamic* Material : TeamMaterials)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(PlaneColorParameterName, TeamColor);
		}
	}
}
```

Passar `TeamMaterials` na chamada de `Build` no `BeginPlay` e aplicar a cor logo depois:

```cpp
	FPlaneMeshBuilder::Build(*this, *CollisionComponent, EPlaneModel::Arcade, BallDiameter,
		PartComponents, SpinningPartComponents, TeamMaterials);
	SetTeamColor(TeamColor);
```

E no fim de `ApplyTuningCVars`:

```cpp
	// Ao contrario das CVars de tuning, esta nao usa a convencao do -1: aqui
	// nao existe "nao definida", o default e um time de verdade.
	const FLinearColor DesiredTeamColor = CVarPlaneTeam.GetValueOnGameThread() == 1
		? FLinearColor(0.85f, 0.15f, 0.15f)    // vermelho: Leste
		: FLinearColor(0.15f, 0.35f, 0.85f);   // azul: Oeste

	if (!DesiredTeamColor.Equals(TeamColor))
	{
		SetTeamColor(DesiredTeamColor);
	}
```

- [ ] **Step 5: Compilar**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FutebolAviaoEditor Win64 Development -Project="C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -WaitMutex
```

Esperado: build com sucesso.

- [ ] **Step 6: Verificar no Play**

Dar Play, abrir o console com `~` e rodar `fa.Plane.Team 1`. O avião tem que virar vermelho **sem a cabine e a hélice mudarem de cor**. Voltar com `fa.Plane.Team 0`.

Se o avião ficar todo cinza, o parâmetro `Color` não foi encontrado — confira o caminho do material. Mandar a captura para o usuário.

- [ ] **Step 7: Commit**

```bash
git add Source/FutebolAviao/Flight/PlaneMeshBuilder.h Source/FutebolAviao/Flight/PlaneMeshBuilder.cpp Source/FutebolAviao/Flight/PlanePawn.h Source/FutebolAviao/Flight/PlanePawn.cpp
git commit -m "feat: paint the team-colored parts of the plane"
```

---

### Task 6: Hélice girando

**Files:**
- Modify: `Source/FutebolAviao/Flight/PlanePawn.cpp`
- Modify: `Source/FutebolAviao/Flight/PlanePawn.h`

**Interfaces:**
- Consumes: `SpinningPartComponents`, preenchido pelo `FPlaneMeshBuilder::Build` na Task 4.
- Produces: nada consumido por tasks posteriores.

- [ ] **Step 1: Acrescentar a constante e girar no Tick**

Em `PlanePawn.h`, junto de `DefaultCollisionRadius`:

```cpp
	// Helice: rapida o bastante pra borrar, nao tanto que vire estroboscopio
	// com o passo do frame.
	static constexpr float PropellerDegreesPerSecond = 1440.f;
```

Em `PlanePawn.cpp`, dentro de `Tick`, logo depois do `AddActorWorldOffset` e antes do bloco de debug:

```cpp
	// Um aviao parado no ar nao parece ligado. A helice e o que da motor.
	// Roda mesmo com o acelerador solto: e sinal de "vivo", nao de empuxo.
	for (UStaticMeshComponent* Component : SpinningPartComponents)
	{
		if (Component)
		{
			Component->AddLocalRotation(FRotator(0.f, 0.f, PropellerDegreesPerSecond * DeltaSeconds));
		}
	}
```

O eixo é o **Roll** porque a hélice gira em torno do X local — o eixo que aponta para o nariz.

- [ ] **Step 2: Compilar**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FutebolAviaoEditor Win64 Development -Project="C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -WaitMutex
```

Esperado: build com sucesso.

- [ ] **Step 3: Verificar no Play**

Dar Play. A hélice do avião arcade gira em torno do eixo do nariz, e não de través. Se ela girar como uma roda de carro, o eixo está errado: trocar o `Roll` por `Yaw` no `FRotator`.

- [ ] **Step 4: Commit**

```bash
git add Source/FutebolAviao/Flight/PlanePawn.h Source/FutebolAviao/Flight/PlanePawn.cpp
git commit -m "feat: spin the propeller so the plane looks alive in the air"
```

---

### Task 7: Trocar de modelo ao vivo e derivar o raio de colisão

**Files:**
- Modify: `Source/FutebolAviao/Flight/PlanePawn.h`
- Modify: `Source/FutebolAviao/Flight/PlanePawn.cpp`

**Interfaces:**
- Consumes: `PlaneModel::CollisionRadiusFor` da Task 3, `FPlaneMeshBuilder::Build`/`Clear` das Tasks 4-5.
- Produces: CVar `fa.Plane.Model`.

Esta é a task que fecha o loop de iteração: trocar de avião e reescalar durante o PIE, sem rebuild.

- [ ] **Step 1: Guardar o estado que dispara a remontagem**

Em `PlanePawn.h`, nos campos privados:

```cpp
	// O que a montagem atual assumiu. Se qualquer um dos dois mudar, as pecas
	// sao destruidas e remontadas -- e isso que permite trocar de aviao e
	// reescalar durante o Play, sem recompilar.
	EPlaneModel CurrentModel = EPlaneModel::Arcade;
	float CurrentBallDiameter = 0.f;

	// Remonta as pecas e reaplica a cor do time.
	void RebuildModel(EPlaneModel Model, float BallDiameter);
```

Com `#include "PlaneModel.h"` no header (ele já inclui `FlightPhysics.h` e `FuelSystem.h`, que são igualmente puros).

Trocar também o `DefaultCollisionRadius`, que deixa de ser um literal:

```cpp
	// Nao e mais um numero solto: sai do modelo, proporcional a bola. Com a
	// bola de hoje (raio 400) da 600, o mesmo valor de antes.
	float CollisionRadius = 600.f;
```

Remover o `static constexpr float DefaultCollisionRadius = 600.f;`. O construtor passa a usar `PlaneModel::CollisionRadiusFor(EPlaneModel::Arcade, FBallPhysicsParams().Radius * 2.f)` para o raio inicial da esfera.

- [ ] **Step 2: Acrescentar a CVar do modelo**

No `namespace` anônimo de `PlanePawn.cpp`:

```cpp
	// int32 e sem a convencao do -1: aqui "nao definida" nao quer dizer nada,
	// o default e um modelo de verdade.
	TAutoConsoleVariable<int32> CVarPlaneModel(TEXT("fa.Plane.Model"), 2, TEXT("Modelo do aviao: 0 = delta, 1 = warbird, 2 = arcade."));
```

E um helper, no mesmo namespace:

```cpp
	EPlaneModel ModelFromCVar(int32 Value)
	{
		switch (Value)
		{
		case 0:  return EPlaneModel::Delta;
		case 1:  return EPlaneModel::Warbird;
		default: return EPlaneModel::Arcade;
		}
	}
```

- [ ] **Step 3: Extrair `RebuildModel` e chamá-lo do BeginPlay**

```cpp
void APlanePawn::RebuildModel(EPlaneModel Model, float BallDiameter)
{
	FPlaneMeshBuilder::Build(*this, *CollisionComponent, Model, BallDiameter,
		PartComponents, SpinningPartComponents, TeamMaterials);
	SetTeamColor(TeamColor);

	CurrentModel = Model;
	CurrentBallDiameter = BallDiameter;
}
```

E o `BeginPlay` passa a ser:

```cpp
	const float BallDiameter = FBallPhysicsParams().Radius * 2.f;
	RebuildModel(ModelFromCVar(CVarPlaneModel.GetValueOnGameThread()), BallDiameter);
```

- [ ] **Step 4: Remontar quando o modelo ou a bola mudarem**

No fim de `ApplyTuningCVars`, substituir o bloco atual do raio de colisão:

```cpp
	CollisionRadius = DefaultCollisionRadius;
	Tuning::Apply(CVarPlaneCollisionRadius, CollisionRadius);
```

por:

```cpp
	// O diametro da bola sai da MESMA CVar que reescala a malha da bola. Ler o
	// default puro e ignorar a CVar faria o aviao manter o tamanho antigo
	// quando a bola crescesse no meio do playtest, e a proporcao -- que e a
	// regra deste modelo todo -- quebraria em silencio.
	FBallPhysicsParams BallParams;
	Tuning::Apply(CVarBallRadius, BallParams.Radius);
	const float BallDiameter = BallParams.Radius * 2.f;

	const EPlaneModel DesiredModel = ModelFromCVar(CVarPlaneModel.GetValueOnGameThread());
	if (DesiredModel != CurrentModel || BallDiameter != CurrentBallDiameter)
	{
		RebuildModel(DesiredModel, BallDiameter);
	}

	CollisionRadius = PlaneModel::CollisionRadiusFor(CurrentModel, BallDiameter);
	Tuning::Apply(CVarPlaneCollisionRadius, CollisionRadius);

	if (CollisionComponent)
	{
		CollisionComponent->SetSphereRadius(CollisionRadius);
	}
```

`CVarBallRadius` está declarado no `namespace` anônimo de `BallActor.cpp`, que é **outra unidade de tradução** — não dá para referenciá-lo de `PlanePawn.cpp`. Declarar aqui uma segunda `TAutoConsoleVariable` com o mesmo nome faria o registro da CVar falhar em runtime. Buscar a CVar já registrada pelo nome:

```cpp
	// A CVar e registrada por BallActor.cpp; aqui so lemos o valor dela.
	if (IConsoleVariable* BallRadiusVar = IConsoleManager::Get().FindConsoleVariable(TEXT("fa.Ball.Radius")))
	{
		const float Value = BallRadiusVar->GetFloat();
		if (Value >= 0.f)
		{
			BallParams.Radius = Value;
		}
	}
```

Use este bloco no lugar da linha `Tuning::Apply(CVarBallRadius, BallParams.Radius);`.

- [ ] **Step 5: Compilar**

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FutebolAviaoEditor Win64 Development -Project="C:\Users\cauad\Desktop\dev\jogao\FutebolAviao.uproject" -WaitMutex
```

Esperado: build com sucesso.

- [ ] **Step 6: Rodar as suítes puras**

```bash
powershell -ExecutionPolicy Bypass -File Tools/run-tests.ps1
```

Esperado: as cinco suítes verdes.

- [ ] **Step 7: Verificar a troca ao vivo**

Dar Play e, no console:

- `fa.Plane.Model 0` → vira o caça delta, com a asa em seta.
- `fa.Plane.Model 1` → vira o monomotor a hélice.
- `fa.Plane.Model 2` → volta ao arcade.
- `fa.Ball.Radius 800` → a bola dobra de tamanho **e o avião dobra junto**. Este é o teste que prova que a proporção é real e não decorativa.
- `fa.Ball.Radius -1` → os dois voltam ao tamanho normal.

Tirar uma captura de cada um dos três modelos e mandar para o usuário.

- [ ] **Step 8: Commit**

```bash
git add Source/FutebolAviao/Flight/PlanePawn.h Source/FutebolAviao/Flight/PlanePawn.cpp
git commit -m "feat: swap plane models live and derive the collision radius from the ball"
```

---

## Notas para quem executa

**Ordem importa.** As Tasks 1-3 são puras e testáveis; as Tasks 4-7 tocam a engine e são verificadas a olho. Não comece a Task 4 antes de `run-tests.ps1` estar verde, porque a partir dali qualquer erro de aritmética vira "o avião ficou torto" em vez de um assert com nome.

**O único risco de sinal do plano** é o pitch do conserto de eixo em `PlaneModel::PartPitchDeg`. Ele está isolado numa função de uma linha, coberto por um teste, e o sintoma é inequívoco: o avião voa apontando para cima. Se acontecer, é trocar `+ 90.f` por `- 90.f` e ajustar o teste correspondente.

**As tabelas de peças são chute informado.** A spec diz isso explicitamente. Depois da captura da Task 4, é esperado que o usuário peça ajustes de proporção — e ajustar é editar números numa tabela e rodar os testes, não redesenhar nada.

**O que este plano deliberadamente não faz:** o avião continua deslizando ao raspar na parede, e a bola continua vendo o avião como uma esfera só. As duas coisas ficaram de fora da spec por decisão do usuário. O que o plano entrega é a estrutura que torna as duas mudanças baratas depois — `GetCollision` já devolve lista, e o raio já é dado em vez de literal.
