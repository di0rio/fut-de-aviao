#include "PlaneMeshBuilder.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

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
		// nome fixo. Criar em runtime e o que permite montar o aviao a partir
		// de qualquer EPlaneModel escolhido no BeginPlay.
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
