#include "PlanePawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"

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
	SpawnTransform = GetActorTransform();
	ResetFlightStateTo(SpawnTransform);
}

void APlanePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const bool bBoostActive = FuelSystem.ResolveBoost(FuelState, bBoostInput);
	FuelSystem.Update(FuelState, bBoostActive, DeltaSeconds);

	// Explodiu neste frame: some da arena e para de voar.
	if (FuelState.bIsDestroyed && !bWasDestroyed)
	{
		bWasDestroyed = true;
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}

	// Respawnou neste frame: volta ao ponto inicial, parado e visivel.
	if (!FuelState.bIsDestroyed && bWasDestroyed)
	{
		bWasDestroyed = false;
		SetActorTransform(SpawnTransform);
		ResetFlightStateTo(SpawnTransform);

		// Os quatro eixos sao atribuidos (nao acumulados) pelos handlers de
		// BindAxis, e o pawn ticka depois do PlayerController, entao eles ja
		// guardam o valor atual do frame - zerar aqui e inocuo hoje, mas fica
		// certo se o input for desabilitado enquanto o ator esta destruido.
		// bBoostInput ja e outra historia: e orientado a evento (IE_Pressed/
		// IE_Released). Esconder o ator NAO suprime input - a causa real e que
		// um jogador que segura a tecla de boost durante toda a morte nunca
		// gera um IE_Released, entao a flag continua true atravessando o
		// respawn inteiro. Sem isto o aviao renasce ainda "boostando",
		// reesvazia o tanque e reexplode em loop.
		ThrottleInput = 0.f;
		PitchInput = 0.f;
		YawInput = 0.f;
		RollInput = 0.f;
		bBoostInput = false;

		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
	}

	if (FuelState.bIsDestroyed)
	{
		return; // fora da jogada: sem fisica de voo enquanto o timer roda
	}

	FlightPhysics.Update(FlightState, ThrottleInput, PitchInput, YawInput, RollInput, bBoostActive, DeltaSeconds);

	const FRotator NewRotation(FlightState.PitchDeg, FlightState.YawDeg, FlightState.RollDeg);
	SetActorRotation(NewRotation);

	AddActorWorldOffset(FVector(FlightState.Velocity.X, FlightState.Velocity.Y, FlightState.Velocity.Z) * DeltaSeconds, true);

	// Sem HUD ainda: o combustivel aparece como texto de debug pra dar pra jogar a Task 4.
#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		// Chave por pawn (nao um literal fixo): com quatro avioes locais, um
		// literal fixo faz todos escreverem no mesmo slot e so um aparece.
		const uint64 DebugKey = static_cast<uint64>(GetUniqueID());
		GEngine->AddOnScreenDebugMessage(DebugKey, 0.05f, FColor::Yellow,
			FString::Printf(TEXT("Fuel %.0f  Speed %.0f%s"), FuelState.Fuel, FFlightPhysics::GetSpeed(FlightState), bBoostActive ? TEXT("  BOOST") : TEXT("")));
	}
#endif
}

void APlanePawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Throttle", this, &APlanePawn::HandleThrottleInput);
	PlayerInputComponent->BindAxis("Pitch", this, &APlanePawn::HandlePitchInput);
	PlayerInputComponent->BindAxis("Yaw", this, &APlanePawn::HandleYawInput);
	PlayerInputComponent->BindAxis("Roll", this, &APlanePawn::HandleRollInput);

	PlayerInputComponent->BindAction("Boost", IE_Pressed, this, &APlanePawn::HandleBoostPressed);
	PlayerInputComponent->BindAction("Boost", IE_Released, this, &APlanePawn::HandleBoostReleased);
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

void APlanePawn::HandleBoostPressed()
{
	bBoostInput = true;
}

void APlanePawn::HandleBoostReleased()
{
	bBoostInput = false;
}

void APlanePawn::SetSpawnTransform(const FTransform& NewSpawnTransform)
{
	SpawnTransform = NewSpawnTransform;
}

void APlanePawn::ResetFlightStateTo(const FTransform& Transform)
{
	const FRotator Rotation = Transform.Rotator();
	FlightState = FFlightPhysicsState();
	FlightState.PitchDeg = Rotation.Pitch;
	FlightState.YawDeg = Rotation.Yaw;
	FlightState.RollDeg = Rotation.Roll;
}
