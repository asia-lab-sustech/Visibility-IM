// Fill out your copyright notice in the Description page of Project Settings.


#include "TriangleActor.h"
#include "ConstructorHelpers.h"
#include "Components/LineBatchComponent.h"
#include "UnrealNetwork.h"

#pragma optimize( "", off )
// Sets default values
ATriangleActor::ATriangleActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FObjectFinder<UMaterial> MaterialHandSpriteLoader(_T("Material'/Game/StarterContent/Materials/M_ProceduralMesh_1-1.M_ProceduralMesh_1-1'"));

	Material = (UMaterial*)MaterialHandSpriteLoader.Object;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TriangleMesh"));

	SetRootComponent(Mesh);

	if (Role == ROLE_Authority)
	{
		SetReplicates(true);
	}
}

// Called when the game starts or when spawned
void ATriangleActor::BeginPlay()
{
	Super::BeginPlay();
	
	DrawTriangle();
}

// Called every frame
void ATriangleActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATriangleActor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	DOREPLIFETIME(ATriangleActor, Triangle);
// 	DOREPLIFETIME(ATriangleActor, Mesh);
}

void ATriangleActor::SetTriangle(FTriangle& OutTriangle)
{
	Triangle = OutTriangle;
}

void ATriangleActor::DrawTriangle()
{
// 	if (Role == ROLE_Authority)
// 	{
// 		UE_LOG(LogTemp, Log, TEXT("ROLE_Authority"));
// 	}
// 	else
// 	{
// 		UE_LOG(LogTemp, Log, TEXT("ROLE_Client"));
// 	}

	FTriangleNode n1 = Triangle.n1;
	FTriangleNode n2 = Triangle.n2;
	FTriangleNode n3 = Triangle.n3;

	TArray<FVector> Vertices;
	FVector Center = FVector((n1.x + n2.x + n3.x) / 3, (n1.y + n2.y + n3.y) / 3, 0.0);
	Vertices.Add(FVector(n1.x, n1.y, 0.0) - Center);
	Vertices.Add(FVector(n2.x, n2.y, 0.0) - Center);
	Vertices.Add(FVector(n3.x, n3.y, 0.0) - Center);
	TArray<int32> Triangles;
	Triangles.Add(0);
	Triangles.Add(1);
	Triangles.Add(2);
	Triangles.Add(0);
	Triangles.Add(2);
	Triangles.Add(1);
	TArray<FLinearColor> VertexColors;
	VertexColors.Add(FLinearColor::Red);
	VertexColors.Add(FLinearColor::Red);
	VertexColors.Add(FLinearColor::Red);

	const TArray<FVector> Normals;
	const TArray<FVector2D> UV0;
	const TArray<FProcMeshTangent> Tangents;

	Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
	Mesh->CreateDynamicMaterialInstance(0, Material, TEXT("TriangleMaterial"));

	DrawPersistentLine(GetWorld(), FVector(n1.x, n1.y, 200.0), FVector(n2.x, n2.y, 200.0), FColor::Orange);
	DrawPersistentLine(GetWorld(), FVector(n1.x, n1.y, 200.0), FVector(n3.x, n3.y, 200.0), FColor::Orange);
	DrawPersistentLine(GetWorld(), FVector(n3.x, n3.y, 200.0), FVector(n2.x, n2.y, 200.0), FColor::Orange);
}

static void DrawPersistentLine(UWorld *World, const FVector &Start, const FVector &End, const FColor &Color)
{
	if (World && World->PersistentLineBatcher)
	{
		World->PersistentLineBatcher->DrawLine(Start, End, Color, SDPG_World, 10.0);
	}
}

#pragma optimize( "", on )
