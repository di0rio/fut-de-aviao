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

	// Toda primitiva de /Engine/BasicShapes tem 100 de lado.
	const float BasicShapeSize = 100.f;

	bool IsRound(EPlaneShape Shape)
	{
		return Shape == EPlaneShape::Cylinder || Shape == EPlaneShape::Cone;
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
}

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
