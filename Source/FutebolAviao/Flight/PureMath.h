#pragma once

#include <cmath>

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

	// Vetor proprio: as classes puras nao podem usar FVector. Os adapters
	// convertem na fronteira.
	struct FPureVector
	{
		float X = 0.f;
		float Y = 0.f;
		float Z = 0.f;
	};

	inline float Dot(const FPureVector& A, const FPureVector& B)
	{
		return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
	}

	inline float Length(const FPureVector& V)
	{
		return std::sqrt(Dot(V, V));
	}

	inline FPureVector Add(const FPureVector& A, const FPureVector& B)
	{
		FPureVector Result;
		Result.X = A.X + B.X;
		Result.Y = A.Y + B.Y;
		Result.Z = A.Z + B.Z;
		return Result;
	}

	inline FPureVector Subtract(const FPureVector& A, const FPureVector& B)
	{
		FPureVector Result;
		Result.X = A.X - B.X;
		Result.Y = A.Y - B.Y;
		Result.Z = A.Z - B.Z;
		return Result;
	}

	inline FPureVector Scale(const FPureVector& V, float Factor)
	{
		FPureVector Result;
		Result.X = V.X * Factor;
		Result.Y = V.Y * Factor;
		Result.Z = V.Z * Factor;
		return Result;
	}

	// Devolve o vetor unitario. Vetor de comprimento desprezivel devolve zero --
	// quem chama decide o fallback, porque a direcao certa depende do contexto.
	inline FPureVector Normalized(const FPureVector& V)
	{
		const float Len = Length(V);
		if (Len < 0.0001f)
		{
			return FPureVector();
		}
		return Scale(V, 1.f / Len);
	}
}
