#pragma once

#include "PureMath.h"

// Descricao dos modelos de aviao como DADO PURO -- sem nenhum header da
// Unreal, igual FlightPhysics e BallPhysics. Quem vira componente e material
// e o adapter (PlaneMeshBuilder), do lado da engine.
//
// TODAS as medidas aqui estao em DIAMETROS DE BOLA, nunca em centimetros. E
// isso que faz "o aviao e proporcional a bola" ser uma propriedade
// verificavel do dado, em vez de um comentario que envelhece: mudar o raio da
// bola reescala os tres avioes junto, e a suite trava a proporcao.
//
// Eixos locais: X pro nariz, Y pra asa direita, Z pra cima -- a mesma
// convencao que FFlightPhysicsState ja usa.

enum class EPlaneShape
{
	Cube,
	Cylinder,
	Cone,
	Sphere,
};

enum class EPlaneTint
{
	Team,    // recebe a cor do time
	Dark,    // fixa: nariz, helice, bocal
	Light,   // fixa: cabine, spinner
};

enum class EPlaneModel
{
	Delta,
	Warbird,
	Arcade,
};

struct FPlanePart
{
	EPlaneShape Shape = EPlaneShape::Cube;

	// Centro da peca, relativo ao centro do aviao, em diametros de bola.
	PureMath::FPureVector Offset;

	// Tamanho TOTAL da peca nos eixos do AVIAO (X = comprimento, Y = largura,
	// Z = altura), em diametros de bola.
	//
	// Nao e o tamanho no frame do mesh. Cilindro e cone de /Engine/BasicShapes
	// nascem com o eixo em Z, e deitar a peca na direcao do nariz e problema
	// do adapter: ele faz esse conserto sozinho a partir do Shape (ver
	// PartMeshScale e PartRotation, na Task 3). Se o conserto de eixo morasse
	// aqui, Size deixaria de significar comprimento e cada linha de tabela
	// poderia erra-lo por conta propria.
	PureMath::FPureVector Size;

	// Rotacao de DESIGN, em graus -- NAO inclui o conserto de eixo das
	// primitivas. Hoje so a asa enflechada do delta usa isto.
	float PitchDeg = 0.f;
	float YawDeg = 0.f;
	float RollDeg = 0.f;

	EPlaneTint Tint = EPlaneTint::Team;

	// Helice: gira em torno do X local a cada tick.
	bool bSpins = false;
};

struct FPlaneCollisionSphere
{
	PureMath::FPureVector Offset;   // em diametros de bola
	float Radius = 0.f;             // em diametros de bola
};

// Arrays de tamanho fixo devolvidos por valor: o codigo puro nao aloca e nao
// usa STL, pra continuar compilando tanto dentro da engine quanto nas suites
// standalone de Tools/.
struct FPlaneParts
{
	static constexpr int MaxParts = 16;
	FPlanePart Parts[MaxParts];
	int Count = 0;
};

struct FPlaneCollision
{
	static constexpr int MaxSpheres = 4;
	FPlaneCollisionSphere Spheres[MaxSpheres];
	int Count = 0;
};

namespace PlaneModel
{
	FPlaneParts GetParts(EPlaneModel Model);

	// Colisao contra a bola, em espaco local. Hoje TODO modelo devolve
	// exatamente uma esfera, centrada, de raio 0.75 -- comportamento
	// identico ao do cone que existia antes. A assinatura ja e plural de
	// proposito: dar a asa uma esfera separada da fuselagem depois vira
	// acrescentar linhas nesta tabela e um laco no ABallActor, sem mexer
	// em arquitetura.
	FPlaneCollision GetCollision(EPlaneModel Model);
}
