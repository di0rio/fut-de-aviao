#include "BallActor.h"
#include "PlanePawn.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "../Tuning/TuningCVars.h"

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

	TAutoConsoleVariable<float> CVarBallMaxSpeed(TEXT("fa.Ball.MaxSpeed"), -1.f, TEXT("Teto de velocidade da bola, cm/s. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallGravity(TEXT("fa.Ball.Gravity"), -1.f, TEXT("Gravidade da bola, cm/s2. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallDrag(TEXT("fa.Ball.Drag"), -1.f, TEXT("Fracao da velocidade perdida por segundo. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallRestitution(TEXT("fa.Ball.Restitution"), -1.f, TEXT("Quanto da velocidade sobra depois de quicar. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallHitTransfer(TEXT("fa.Ball.HitTransfer"), -1.f, TEXT("Quanto da velocidade do aviao vira impulso. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallMinKick(TEXT("fa.Ball.MinKick"), -1.f, TEXT("Impulso minimo de um toque de raspao. -1 usa o default."));
	TAutoConsoleVariable<float> CVarBallRadius(TEXT("fa.Ball.Radius"), -1.f, TEXT("Raio da bola. Reescala a malha junto. -1 usa o default."));
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
	ApplyTuningCVars();

	// Primeiro colhe todos os avioes que estao tocando a bola neste frame, em
	// vez de resolver aviao a aviao dentro do proprio laco do TActorIterator.
	// A ordem desse iterador nao e uma chave estavel entre frames -- com mais
	// de um aviao sobrepondo, o ultimo a aparecer nela decidia onde a bola
	// era reposicionada (ApplyHit reescreve a posicao inteira), enquanto o
	// impulso de cada um ja tinha acumulado na velocidade. Isso e exatamente o
	// tipo de resolucao de contato nao-deterministica que a fisica pura desta
	// arquitetura existe para evitar (ela precisa ser rederivavel igual dos
	// dois lados, cliente e servidor). Um aviao so (o caso de hoje) nao muda
	// de comportamento: com um unico candidato, ordenar e nao ordenar da no
	// mesmo, e ApplyImpulse + PushOutOf abaixo reproduzem exatamente o que
	// ApplyHit fazia sozinho.
	TArray<APlanePawn*> OverlappingPlanes;
	for (TActorIterator<APlanePawn> It(GetWorld()); It; ++It)
	{
		APlanePawn* Plane = *It;
		if (!Plane || !Plane->IsFlying())
		{
			continue;   // aviao destruido, aguardando respawn, nao acerta bola
		}

		const PureMath::FPureVector PlanePosition = ToPure(Plane->GetActorLocation());
		if (BallPhysics.IsOverlapping(BallState, PlanePosition, Plane->GetCollisionRadius()))
		{
			OverlappingPlanes.Add(Plane);
		}
	}

	if (OverlappingPlanes.Num() > 0)
	{
		// GetUniqueID() como chave estavel: ja usado assim em
		// APlanePawn::Tick (chave da mensagem de debug por aviao). Ao
		// contrario da ordem do TActorIterator, nao muda de frame pra frame
		// sem que nenhum aviao tenha mudado.
		OverlappingPlanes.Sort([](const APlanePawn& A, const APlanePawn& B)
			{
				return A.GetUniqueID() < B.GetUniqueID();
			});

		// Acumula o impulso de cada aviao sobreposto antes de tocar na
		// posicao da bola -- assim nenhum deles desfaz o empurrao do anterior.
		for (APlanePawn* Plane : OverlappingPlanes)
		{
			BallPhysics.ApplyImpulse(BallState, ToPure(Plane->GetActorLocation()), Plane->GetPureVelocity());
		}

		// O empurrao pra fora da sobreposicao acontece uma unica vez, relativo
		// ao aviao de menor GetUniqueID() entre os que se sobrepuseram --
		// escolha deterministica e estavel entre frames. Com um aviao so
		// (hoje), e o unico candidato: resultado identico ao ApplyHit antigo.
		APlanePawn* PushOutPlane = OverlappingPlanes[0];
		BallPhysics.PushOutOf(BallState, ToPure(PushOutPlane->GetActorLocation()), PushOutPlane->GetPureVelocity(), PushOutPlane->GetCollisionRadius());
	}

	BallPhysics.Update(BallState, DeltaSeconds);
	SetActorLocation(ToUnreal(BallState.Position));
}

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
