// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriangleStruct.h"
#include "ProceduralMeshComponent.h"
#include "TriangleActor.generated.h"

UCLASS()
class FPS_API ATriangleActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATriangleActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterial* Material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	struct FTriangle Triangle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UProceduralMeshComponent* Mesh;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	void SetTriangle(FTriangle& OutTriangle);
	void DrawTriangle();
};


static void DrawPersistentLine(UWorld *World, const FVector &Start, const FVector &End, const FColor &Color);
