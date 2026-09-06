#pragma once

// Descricao geometrica da arena, compartilhada por FBallPhysicsParams e
// FMatchParams. Antes cada um declarava a sua propria copia de ArenaHalfX (e
// so FMatchParams sabia da boca do gol) -- foi assim que a fisica da bola
// ficou quicando bem onde deveria ser a entrada do gol, porque ela nao tinha
// como saber onde essa entrada ficava. Uma unica fonte de verdade evita essa
// divergencia de novo.
//
// So-header, sem .cpp: as suites standalone em Tools/ compilam apenas
// <Nome>Tests.cpp mais <Nome>.cpp, entao nao ha onde linkar um
// ArenaGeometry.cpp separado. Sem nenhum header da Unreal, igual as outras
// classes puras.
struct FArenaGeometry
{
	// Arena: caixa fechada. X e Y sao metades; o chao e Z=0.
	float ArenaHalfX = 10000.f;
	float ArenaHalfY = 6000.f;
	float ArenaCeilingZ = 5000.f;

	// Boca do gol: um retangulo aberto nas duas paredes de fundo (X =
	// +-ArenaHalfX), centrado em Y=0 e indo do chao ate GoalHeightZ.
	float GoalHalfWidthY = 1500.f;
	float GoalHeightZ = 2000.f;
};
