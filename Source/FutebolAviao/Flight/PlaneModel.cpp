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
