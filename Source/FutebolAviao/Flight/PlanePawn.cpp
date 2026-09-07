#include "PlanePawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "../Tuning/TuningCVars.h"

namespace
{
	TAutoConsoleVariable<float> CVarPlaneMaxSpeed(TEXT("fa.Plane.MaxSpeed"), -1.f, TEXT("Velocidade maxima em cruzeiro, cm/s. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneBoostMaxSpeed(TEXT("fa.Plane.BoostMaxSpeed"), -1.f, TEXT("Teto de velocidade no boost, cm/s. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneAcceleration(TEXT("fa.Plane.Acceleration"), -1.f, TEXT("Aceleracao, cm/s2. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneBoostAcceleration(TEXT("fa.Plane.BoostAcceleration"), -1.f, TEXT("Aceleracao no boost, cm/s2. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneDrag(TEXT("fa.Plane.Drag"), -1.f, TEXT("Arrasto com acelerador solto, cm/s2. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneTurnRate(TEXT("fa.Plane.TurnRate"), -1.f, TEXT("Taxa de pitch E yaw juntos, graus/s. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneVelocityAlign(TEXT("fa.Plane.VelocityAlign"), -1.f, TEXT("Quao rapido a velocidade persegue o nariz. Valor alto (1000) desliga a inercia. -1 usa o default."));
	TAutoConsoleVariable<float> CVarPlaneCollisionRadius(TEXT("fa.Plane.CollisionRadius"), -1.f, TEXT("Raio esferico do aviao pra colisao com a bola. -1 usa o default."));

	TAutoConsoleVariable<float> CVarFuelTankCapacity(TEXT("fa.Fuel.TankCapacity"), -1.f, TEXT("Tamanho do tanque. -1 usa o default."));
	TAutoConsoleVariable<float> CVarFuelBoostDrain(TEXT("fa.Fuel.BoostDrain"), -1.f, TEXT("Consumo do boost por segundo. -1 usa o default."));
	TAutoConsoleVariable<float> CVarFuelRegen(TEXT("fa.Fuel.Regen"), -1.f, TEXT("Regeneracao passiva por segundo. -1 usa o default."));
	TAutoConsoleVariable<float> CVarFuelRespawnSeconds(TEXT("fa.Fuel.RespawnSeconds"), -1.f, TEXT("Segundos fora da jogada depois de explodir. -1 usa o default."));
}

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
		MeshComponent->SetRelativeScale3D(FVector(12.f, 6.f, 6.f));
		MeshComponent->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	}

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 2500.f;
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
	ApplyTuningCVars();

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

void APlanePawn::ApplyTuningCVars()
{
	// Parte SEMPRE dos defaults puros, nunca dos params atuais. Se lesse os
	// atuais, um valor setado por CVar ficaria grudado pra sempre: voltar a
	// CVar pra -1 nao teria como restaurar o default.
	FFlightPhysicsParams FlightParams;
	Tuning::Apply(CVarPlaneMaxSpeed, FlightParams.MaxSpeed);
	Tuning::Apply(CVarPlaneBoostMaxSpeed, FlightParams.BoostMaxSpeed);
	Tuning::Apply(CVarPlaneAcceleration, FlightParams.Acceleration);
	Tuning::Apply(CVarPlaneBoostAcceleration, FlightParams.BoostAcceleration);
	Tuning::Apply(CVarPlaneDrag, FlightParams.Drag);
	Tuning::Apply(CVarPlaneVelocityAlign, FlightParams.VelocityAlignPerSec);

	// TurnRate mexe nos dois eixos juntos: eles sao mantidos iguais de
	// proposito, pra mirar em 3D ser simetrico.
	Tuning::Apply(CVarPlaneTurnRate, FlightParams.PitchRateDegPerSec);
	Tuning::Apply(CVarPlaneTurnRate, FlightParams.YawRateDegPerSec);
	FlightPhysics.SetParams(FlightParams);

	FFuelParams TunedFuel;
	Tuning::Apply(CVarFuelTankCapacity, TunedFuel.TankCapacity);
	Tuning::Apply(CVarFuelBoostDrain, TunedFuel.BoostDrainPerSec);
	Tuning::Apply(CVarFuelRegen, TunedFuel.PassiveRegenPerSec);
	Tuning::Apply(CVarFuelRespawnSeconds, TunedFuel.RespawnSeconds);

	// Piso de seguranca: capacidade 0 (ou negativa) trava o aviao destruido
	// pra sempre. ClampValue(Fuel, 0, TankCapacity) prende o combustivel em
	// 0 -> destruido; o respawn calcula TankCapacity * RespawnFuelFraction,
	// que tambem da 0 com capacidade 0 -> destruido nascendo, em loop, sem
	// nenhum valor digitado no CVar conseguir tirar o jogador dali de novo.
	TunedFuel.TankCapacity = FMath::Max(TunedFuel.TankCapacity, 1.f);

	FuelSystem.SetParams(TunedFuel);

	CollisionRadius = DefaultCollisionRadius;
	Tuning::Apply(CVarPlaneCollisionRadius, CollisionRadius);
}

void APlanePawn::ResetFlightStateTo(const FTransform& Transform)
{
	const FRotator Rotation = Transform.Rotator();
	FlightState = FFlightPhysicsState();
	FlightState.PitchDeg = Rotation.Pitch;
	FlightState.YawDeg = Rotation.Yaw;
	FlightState.RollDeg = Rotation.Roll;
}
