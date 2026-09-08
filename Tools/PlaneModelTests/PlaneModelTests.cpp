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

	// O delta e o unico com asa enflechada; o warbird tem fuselagem cilindrica
	// fina (aerodinamica esportiva) enquanto o arcade tem fuselagem grossa
	// (design robusto). Tres propriedades que separam os tres dois a dois:
	// delta vs todos pela asa, warbird vs arcade pela fuselagem.
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

	// Warbird fuselage is thin (0.32); if Warbird were misaligned to Arcade,
	// it would inherit Arcade fuselage (0.42). This catches that aliasing.
	const bool bWarbirdFuselageThinnerThanArcade = Warbird.Parts[0].Size.Y < Arcade.Parts[0].Size.Y;

	assert(bDeltaHasSweptWing);
	assert(!bDeltaHasPropeller);
	assert(bWarbirdHasPropeller);
	assert(!bArcadeHasSweptWing);
	assert(bWarbirdFuselageThinnerThanArcade);
	printf("Test_TheThreeModelsAreDifferentFromEachOther passed\n");
}

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

	Test_CollisionRadiusMatchesTheValueAlreadyInThePawn();
	Test_ThePlaneGrowsWithTheBall();
	Test_PartLocationScalesTheOffsetByTheBallDiameter();
	Test_BoxPartScalesStraightFromItsSize();
	Test_RoundPartSwapsAxesBecauseItIsBornAlongZ();
	Test_BoxPartIsNotPitchedByTheAxisFixUp();

	printf("All tests passed\n");
	return 0;
}
