#pragma once

// Helpers matematicos puros, sem nenhum header da Unreal. Existem porque as
// classes puras (FuelSystem, FlightPhysics) nao podem usar FMath::Clamp nem
// qualquer outra coisa da engine - ver a regra de fronteira no topo de
// FlightPhysics.h/FuelSystem.h. So-header, sem .cpp: as suites standalone em
// Tools/ compilam apenas <Nome>Tests.cpp mais <Nome>.cpp, entao nao ha onde
// linkar um PureMath.cpp separado.
namespace PureMath
{
	inline float ClampValue(float Value, float Min, float Max)
	{
		if (Value < Min) return Min;
		if (Value > Max) return Max;
		return Value;
	}

	inline float WrapDegrees(float Degrees)
	{
		float Wrapped = Degrees;
		while (Wrapped >= 360.f) Wrapped -= 360.f;
		while (Wrapped < 0.f) Wrapped += 360.f;
		return Wrapped;
	}
}
