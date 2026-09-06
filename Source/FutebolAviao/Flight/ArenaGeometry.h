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
	float ArenaHalfX = 20000.f;      // 400m de comprimento
	float ArenaHalfY = 12000.f;      // 240m de largura
	float ArenaCeilingZ = 12000.f;   // 120m de altura

	// Boca do gol: um retangulo aberto nas duas paredes de fundo (X =
	// +-ArenaHalfX), centrado em Y=0 e indo do chao ate GoalHeightZ.
	float GoalHalfWidthY = 3000.f;   // boca de 60m
	float GoalHeightZ = 4000.f;      // 40m de altura
};
