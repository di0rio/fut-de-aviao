#pragma once

#include "CoreMinimal.h"
#include "PlaneModel.h"

class AActor;
class USceneComponent;
class UStaticMeshComponent;

// Adapter que transforma a descricao pura de PlaneModel em componentes de
// verdade. Mora do lado da engine: PlaneModel nao pode incluir header da
// Unreal, e APlanePawn nao deveria crescer mais linhas de montagem de malha
// em cima do que ja cuida (voo, input, combustivel, tuning).
//
// TODAS as pecas criadas aqui sao SEM COLISAO, de proposito. O visual do
// aviao nao participa de nenhuma fisica -- quem colide e a USphereComponent
// do pawn. E isso que faz trocar de modelo, engordar uma asa ou acrescentar
// uma peca nunca mudar como o aviao bate em nada.
struct FPlaneMeshBuilder
{
	static void Build(AActor& Owner, USceneComponent& Attach, EPlaneModel Model, float BallDiameter,
		TArray<UStaticMeshComponent*>& OutParts, TArray<UStaticMeshComponent*>& OutSpinningParts);

	static void Clear(TArray<UStaticMeshComponent*>& Parts, TArray<UStaticMeshComponent*>& SpinningParts);
};
