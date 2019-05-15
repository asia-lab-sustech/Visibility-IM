#pragma once
#include "CoreMinimal.h"
#include "TriangleStruct.generated.h"

USTRUCT(BlueprintType)
struct FTriangleNode
{
	GENERATED_BODY()

		uint32 index;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float x;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float y;
};

USTRUCT(BlueprintType)
struct FTriangle
{
	GENERATED_BODY()

		uint32 index;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FTriangleNode n1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FTriangleNode n2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FTriangleNode n3;
};

USTRUCT(BlueprintType)
struct FPolygon
{
	GENERATED_BODY()

		uint32 index;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FTriangleNode> Nodes;

};