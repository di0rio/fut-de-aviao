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
