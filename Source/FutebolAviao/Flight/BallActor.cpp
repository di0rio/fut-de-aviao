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
