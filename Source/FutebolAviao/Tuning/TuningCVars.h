#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

// Camada de tuning ao vivo. Vive no lado da engine de proposito: as classes
// puras nao podem incluir header da Unreal.
//
// Convencao: cada CVar nasce em -1 ("nao definida") e so sobrescreve o campo
// quando o valor for >= 0. Assim os headers puros continuam sendo a fonte
// unica da verdade dos defaults -- nenhum numero e repetido aqui. Duplicar
// default entre dois lugares foi o que produziu o bug do gol inalcancavel na
// Fase 3, e esta convencao existe pra nao repetir isso.
namespace Tuning
{
	inline void Apply(const TAutoConsoleVariable<float>& Var, float& Field)
	{
		const float Value = Var.GetValueOnGameThread();
		if (Value >= 0.f)
		{
			Field = Value;
		}
	}
}
