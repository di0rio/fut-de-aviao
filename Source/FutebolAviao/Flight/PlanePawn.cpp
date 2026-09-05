#include "PlanePawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"

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
}

void APlanePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FlightPhysics.Update(FlightState, ThrottleInput, PitchInput, YawInput, RollInput, DeltaSeconds);

	const FRotator NewRotation(FlightState.PitchDeg, FlightState.YawDeg, FlightState.RollDeg);
	SetActorRotation(NewRotation);

	const FVector Forward = NewRotation.Vector();
	AddActorWorldOffset(Forward * FlightState.Speed * DeltaSeconds, true);
}

void APlanePawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Throttle", this, &APlanePawn::HandleThrottleInput);
	PlayerInputComponent->BindAxis("Pitch", this, &APlanePawn::HandlePitchInput);
	PlayerInputComponent->BindAxis("Yaw", this, &APlanePawn::HandleYawInput);
	PlayerInputComponent->BindAxis("Roll", this, &APlanePawn::HandleRollInput);
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
