// Fill out your copyright notice in the Description page of Project Settings.

#include "TriangleReplicationGraph.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategoryReplicator.h"
#endif

#include "DrawDebugHelpers.h"
#include "FileHelper.h"
#include "../FPSCharacter.h"
#include "../Public/TriangleActor.h"
#include "GeomTools.h"

#include "GameFramework/PlayerState.h"
#include "Engine/LevelScriptActor.h"
#include <time.h> 



DEFINE_LOG_CATEGORY(LogTriangleReplicationGraph);

#pragma optimize( "", off )


UTriangleReplicationGraph::UTriangleReplicationGraph()
{
	UE_LOG(LogTriangleReplicationGraph, Log, TEXT("UTriangleReplicationGraph"));
}

UTriangleReplicationGraph::~UTriangleReplicationGraph()
{
}

void UTriangleReplicationGraph::ResetGameWorldState()
{
	Super::ResetGameWorldState();
}

void UTriangleReplicationGraph::InitGlobalActorClassSettings()
{
	Super::InitGlobalActorClassSettings();

	// ReplicationGraph stores internal associative data for actor classes. 
	// We build this data here based on actor CDO values.
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		AActor* ActorCDO = Cast<AActor>(Class->GetDefaultObject());
		if (!ActorCDO || !ActorCDO->GetIsReplicated())
		{
			continue;
		}

		// Skip SKEL and REINST classes.
		if (Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_")))
		{
			continue;
		}

		FClassReplicationInfo ClassInfo;

		// Replication Graph is frame based. Convert NetUpdateFrequency to ReplicationPeriodFrame based on Server MaxTickRate.
		ClassInfo.ReplicationPeriodFrame = FMath::Max<uint32>((uint32)FMath::RoundToFloat(NetDriver->NetServerMaxTickRate / ActorCDO->NetUpdateFrequency), 1);

		if (ActorCDO->bAlwaysRelevant || ActorCDO->bOnlyRelevantToOwner)
		{
			ClassInfo.CullDistanceSquared = 0.f;
		}
		else
		{
			ClassInfo.CullDistanceSquared = ActorCDO->NetCullDistanceSquared;
		}

		GlobalActorReplicationInfoMap.SetClassInfo(Class, ClassInfo);
	}
}

void UTriangleReplicationGraph::InitGlobalGraphNodes()
{
	PreAllocateRepList(3, 12);
	PreAllocateRepList(6, 12);
	PreAllocateRepList(64, 512);

	TriangleNode = CreateNewNode<UTriangleReplicationGraphNode_Triangle>();
	AddGlobalGraphNode(TriangleNode);

	AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_ActorList>();
	AddGlobalGraphNode(AlwaysRelevantNode);
}

void UTriangleReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* RepGraphConnection)
{
	Super::InitConnectionGraphNodes(RepGraphConnection);

	UReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantNodeForConnection = CreateNewNode<UReplicationGraphNode_AlwaysRelevant_ForConnection>();
	AddConnectionGraphNode(AlwaysRelevantNodeForConnection, RepGraphConnection);

	// 	AlwaysRelevantForConnectionList.Emplace(RepGraphConnection->NetConnection, AlwaysRelevantNodeForConnection);
}

void UTriangleReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo & ActorInfo, FGlobalActorReplicationInfo & GlobalInfo)
{
	if (ActorInfo.Class == APlayerState::StaticClass() || ActorInfo.Class == AReplicationGraphDebugActor::StaticClass() || ActorInfo.Class == ALevelScriptActor::StaticClass() 
#if WITH_GAMEPLAY_DEBUGGER
		|| ActorInfo.Class == AGameplayDebuggerCategoryReplicator::StaticClass()
#endif
		)
	{
		return;
	}
	if (ActorInfo.Actor->bAlwaysRelevant)
	{
		AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
	}
	else if (!ActorInfo.Actor->bOnlyRelevantToOwner)
	{
		TriangleNode->AddActor_Dynamic(ActorInfo, GlobalInfo);
	}
}

void UTriangleReplicationGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo & ActorInfo)
{
	if (ActorInfo.Actor->bAlwaysRelevant)
	{
		AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
	}
	else
	{
		TriangleNode->RemoveActor_Dynamic(ActorInfo);
	}
}


// UTriangleReplicationGraphNode_Triangle
UTriangleReplicationGraphNode_Triangle::UTriangleReplicationGraphNode_Triangle()
{
	bRequiresPrepareForReplicationCall = true;

	LoadTriangles();
}


void UTriangleReplicationGraphNode_Triangle::NotifyAddNetworkActor(const FNewReplicatedActorInfo & ActorInfo)
{
	ensureAlwaysMsgf(false, TEXT("UTriangleReplicationGraphNode_Triangle::NotifyAddNetworkActor should not be called directly"));
}

bool UTriangleReplicationGraphNode_Triangle::NotifyRemoveNetworkActor(const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound)
{
	ensureAlwaysMsgf(false, TEXT("UReplicationGraphNode_GridSpatialization2D::NotifyRemoveNetworkActor should not be called directly"));
	return false;
}

void UTriangleReplicationGraphNode_Triangle::NotifyResetAllNetworkActors()
{
	// 	StaticSpatializedActors.Reset();
	DynamicSpatializedActors.Reset();
	Super::NotifyResetAllNetworkActors();
}

void UTriangleReplicationGraphNode_Triangle::GatherActorListsForConnection(const FConnectionGatherActorListParameters & Params)
{
	double StartTime = FPlatformTime::Seconds();
	UTriangleReplicationGraphNode_TriangleCell* TriangleCell = GetTriangleCellForLocation(Params.Viewer.ViewLocation);
	if (TriangleCell)
	{
		if (bUseDistance)
		{
			for (UReplicationGraphNode* ChildNode : AllChildNodes)
			{
				UTriangleReplicationGraphNode_TriangleCell* ChildTriangleCell = (UTriangleReplicationGraphNode_TriangleCell *)ChildNode;
				FTriangleNode n1 = ChildTriangleCell->Triangle.n1;
				FTriangleNode n2 = ChildTriangleCell->Triangle.n2;
				FTriangleNode n3 = ChildTriangleCell->Triangle.n3;
				FVector Center = FVector((n1.x + n2.x + n3.x) / 3, (n1.y + n2.y + n3.y) / 3, 200.0);
				if (ChildTriangleCell)
				{
					const float Dist = (Center - Params.Viewer.ViewLocation).Size();
					if (Dist <= RelevantDistance)
					{
						ChildTriangleCell->GatherActorListsForConnection(Params);
					}
				}
			}
		}
		else
		{
			for (UTriangleReplicationGraphNode_TriangleCell* Cell2 : TriangleCell->RelevantCells)
			{
				Cell2->GatherActorListsForConnection(Params);
			}
		}
	}

	double EndTime = FPlatformTime::Seconds();
	double MatchingTime = (EndTime - StartTime) * 1000;

	MatchingAverage = (MatchingAverage * MatchingCount + MatchingTime) / (MatchingCount + 1);
	MatchingCount++;
	if (MatchingCount == 1000)
	{
		UE_LOG(LogTriangleReplicationGraph, Warning, TEXT("%fms"), MatchingTime);
	}
}

void UTriangleReplicationGraphNode_Triangle::PrepareForReplication()
{
	RG_QUICK_SCOPE_CYCLE_COUNTER(UTriangleReplicationGraphNode_Triangle_PrepareForReplication);
	for (auto& MapIt : DynamicSpatializedActors)
	{
		FActorRepListType& DynamicActor = MapIt.Key;
		FCachedDynamicActorInfo& DynamicActorInfo = MapIt.Value;
		FNewReplicatedActorInfo& ActorInfo = DynamicActorInfo.ActorInfo;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (!IsActorValidForReplicationGather(DynamicActor))
		{
			UE_LOG(LogTriangleReplicationGraph, Warning, TEXT("UReplicationGraphNode_GridSpatialization2D::PrepareForReplication: Dynamic Actor no longer ready for replication"));
			UE_LOG(LogTriangleReplicationGraph, Warning, TEXT("%s"), *GetNameSafe(DynamicActor));
			continue;
		}
#endif

		// Check if this resets spatial bias
		const FVector Location3D = DynamicActor->GetActorLocation();

		if (!bNeedsRebuild)
		{
			UTriangleReplicationGraphNode_TriangleCell* TriangleCell = GetTriangleCellForLocation(Location3D);


			if (TriangleCell && DynamicActorInfo.OwnerCell != TriangleCell)
			{
				if (DynamicActorInfo.OwnerCell != nullptr)
				{
					DynamicActorInfo.OwnerCell->RemoveDynamicActor(ActorInfo);
				}
				TriangleCell->AddDynamicActor(ActorInfo);
				DynamicActorInfo.OwnerCell = TriangleCell;
			}
		}
	}
}

void UTriangleReplicationGraphNode_Triangle::LogNode(FReplicationGraphDebugInfo & DebugInfo, const FString & NodeName) const
{
}

void UTriangleReplicationGraphNode_Triangle::LoadTriangles()
{
	FString GameDir = FPaths::ProjectDir();
	FString NodeFilePath = GameDir + "Content/triangle/wall.1.node";
	TArray<FString> nodeLines;
	FFileHelper::LoadFileToStringArray(nodeLines, *NodeFilePath);

	FString firstLine = nodeLines[0];
	TArray<FString> firstLineArgs;

	int nodeNum, dim, attribute, boundaryMarker;
	firstLine.ParseIntoArrayWS(firstLineArgs);
	nodeNum = FCString::Atoi(*firstLineArgs[0]);
	dim = FCString::Atoi(*firstLineArgs[1]);
	attribute = FCString::Atoi(*firstLineArgs[2]);
	boundaryMarker = FCString::Atoi(*firstLineArgs[3]);

	UE_LOG(LogTriangleReplicationGraph, Warning, TEXT("%i %i %i %i"), nodeNum, dim, attribute, boundaryMarker);

	for (size_t i = 1; i < nodeLines.Num() - 1; i++)
	{
		FString line = nodeLines[i];
		TArray<FString> lineArgs;
		line.ParseIntoArrayWS(lineArgs);
		UE_LOG(LogTemp, Warning, TEXT("%s"), *line);
		float x = FCString::Atof(*lineArgs[1]);
		float y = FCString::Atof(*lineArgs[2]);
		// 		DrawDebugPoint(GetWorld(), FVector(x, y, 200.0), 10, FColor::Orange, true, INT_MAX);
		FTriangleNode node = { i - 1, x, y };
		Nodes.Add(node);
	}

	FString EleFilePath = GameDir + "Content/triangle/wall.1.ele";
	TArray<FString> eleLines;
	FFileHelper::LoadFileToStringArray(eleLines, *EleFilePath);


	for (size_t i = 1; i < eleLines.Num() - 1; i++)
	{
		FString line = eleLines[i];
		TArray<FString> lineArgs;
		line.ParseIntoArrayWS(lineArgs);
		UE_LOG(LogTriangleReplicationGraph, Warning, TEXT("%s"), *line);
		int i1 = FCString::Atoi(*lineArgs[1]);
		int i2 = FCString::Atoi(*lineArgs[2]);
		int i3 = FCString::Atoi(*lineArgs[3]);
		FTriangleNode n1 = Nodes[i1 - 1];
		FTriangleNode n2 = Nodes[i2 - 1];
		FTriangleNode n3 = Nodes[i3 - 1];
		FTriangle t = { i - 1, n1, n2, n3 };
		Triangles.Add(t);
	}

	TArray<FTriangleNode> Hode1Nodes;
	Hode1Nodes.Add(Nodes[4]);
	Hode1Nodes.Add(Nodes[5]);
	Hode1Nodes.Add(Nodes[6]);
	Hode1Nodes.Add(Nodes[7]);
	FPolygon hole1 = { 0, Hode1Nodes };
	TArray<FTriangleNode> Hode2Nodes;
	Hode2Nodes.Add(Nodes[8]);
	Hode2Nodes.Add(Nodes[9]);
	Hode2Nodes.Add(Nodes[10]);
	Hode2Nodes.Add(Nodes[11]);
	FPolygon hole2 = { 1, Hode2Nodes };
	Holes.Add(hole1);
	Holes.Add(hole2);
}

void UTriangleReplicationGraphNode_Triangle::AddNodeNeighbours(UTriangleReplicationGraphNode_TriangleCell* Cell, uint32 NodeIndex)
{
	for (UTriangleReplicationGraphNode_TriangleCell* Neighbour : Node2Triangles[NodeIndex])
	{
		if (!Cell->RelevantCells.Contains(Neighbour))
		{
			Cell->RelevantCells.Add(Neighbour);
		}
	}
}

void UTriangleReplicationGraphNode_Triangle::BuildTriangles()
{
	for (float x = -2200.0; x < 1700; x += 100)
	{
		for (float y = -2000.0; y < 2000; y += 100)
		{
			FVector SpawnLocation = FVector(x, y, 200.0);
// 			AFPSProjectile* Project = GetWorld()->SpawnActor<AFPSProjectile>(SpawnLocation, FRotator(0.0, 0.0, 0.0));
		}
	}

	TMap<uint32, UTriangleReplicationGraphNode_TriangleCell*> Triangle2Cell;
	TMap<uint32, TArray<uint32>> RelevantTriangles;
	for (size_t i = 0; i < Triangles.Num(); i++)
	{
		RelevantTriangles.Add(i, TArray<uint32>());
		RelevantTriangles[i].Add(i);
	}
	if (bUseNeighbour)
	{
		for (size_t i = 0; i < Triangles.Num(); i++)
		{
			FTriangle Triangle = Triangles[i];
			UTriangleReplicationGraphNode_TriangleCell* NodePtr = CreateChildNode<UTriangleReplicationGraphNode_TriangleCell>();
			NodePtr->SetTriangle(Triangles[i]);
			Triangle2Cell.Add(i, NodePtr);
			NodePtr->RelevantCells.Add(NodePtr);
			if (!Node2Triangles.Contains(Triangle.n1.index))
			{
				Node2Triangles.Add(Triangle.n1.index, TArray<UTriangleReplicationGraphNode_TriangleCell*>());
			}
			if (!Node2Triangles.Contains(Triangle.n2.index))
			{
				Node2Triangles.Add(Triangle.n2.index, TArray<UTriangleReplicationGraphNode_TriangleCell*>());
			}
			if (!Node2Triangles.Contains(Triangle.n3.index))
			{
				Node2Triangles.Add(Triangle.n3.index, TArray<UTriangleReplicationGraphNode_TriangleCell*>());
			}
			Node2Triangles[Triangle.n1.index].Add(NodePtr);
			Node2Triangles[Triangle.n2.index].Add(NodePtr);
			Node2Triangles[Triangle.n3.index].Add(NodePtr);
		}

		for (size_t i = 0; i < NeighbourDepth; i++)
		{

			for (UReplicationGraphNode* ChildNode : AllChildNodes)
			{
				UTriangleReplicationGraphNode_TriangleCell* TriangleCell = (UTriangleReplicationGraphNode_TriangleCell *)ChildNode;
				if (TriangleCell)
				{
					size_t Num = TriangleCell->RelevantCells.Num();
					for (size_t j = 0; j < Num; j++)
					{
						UTriangleReplicationGraphNode_TriangleCell* RelevantCell = TriangleCell->RelevantCells[j];
						AddNodeNeighbours(TriangleCell, RelevantCell->Triangle.n1.index);
						AddNodeNeighbours(TriangleCell, RelevantCell->Triangle.n2.index);
						AddNodeNeighbours(TriangleCell, RelevantCell->Triangle.n3.index);
					}
				}
			}
		}
	}
	else
	{
		for (size_t i = 0; i < Triangles.Num(); i++)
		{
			FTriangle Ti = Triangles[i];
			UTriangleReplicationGraphNode_TriangleCell* NodePtr = CreateChildNode<UTriangleReplicationGraphNode_TriangleCell>();
			NodePtr->SetTriangle(Ti);
			Triangle2Cell.Add(i, NodePtr);
			for (size_t j = i + 1; j < Triangles.Num(); j++)
			{
				FTriangle Tj = Triangles[j];
				if (DetectVisibility(Ti, Tj))
				{
					RelevantTriangles[i].Add(j);
					RelevantTriangles[j].Add(i);
				}
			}
		}
		for (UReplicationGraphNode* ChildNode : AllChildNodes)
		{
			UTriangleReplicationGraphNode_TriangleCell* TriangleCell = (UTriangleReplicationGraphNode_TriangleCell *)ChildNode;
			if (TriangleCell)
			{

				FTriangle Triangle = TriangleCell->Triangle;
				for (uint32 Tid : RelevantTriangles[Triangle.index])
				{
					UTriangleReplicationGraphNode_TriangleCell* Cell2 = Triangle2Cell[Tid];
					TriangleCell->RelevantCells.Add(Cell2);
				}
			}
		}
	}
}

bool Detect2LineIntersect(FTriangleNode& a, FTriangleNode& b, FTriangleNode& c, FTriangleNode& d)
{
	if (FMath::Min(a.x, b.x) > FMath::Max(c.x, d.x)
		|| FMath::Min(c.x, d.x) > FMath::Max(a.x, b.x)
		|| FMath::Min(a.y, b.y) > FMath::Max(c.y, d.y)
		|| FMath::Min(c.y, d.y) > FMath::Max(a.y, b.y)) {
		return false;
	}

	FVector2D ca(a.x - c.x, a.y - c.y);
	FVector2D cd(d.x - c.x, d.y - c.y);
	FVector2D cb(b.x - c.x, b.y - c.y);
	FVector2D ba(a.x - b.x, a.y - b.y);
	FVector2D bc = -cb;
	FVector2D bd(d.x - b.x, d.y - b.y);

	if (FVector2D::CrossProduct(ca, cd) * FVector2D::CrossProduct(cd, cb) >= 0 
		&& FVector2D::CrossProduct(bc, ba) * FVector2D::CrossProduct(ba, bd) >= 0)
	{
		return true;
	}
	return false;
}

bool DetectLineIntersectHole(FTriangleNode& node1, FTriangleNode& node2, FPolygon& Hole)
{
	if (node1.index == node2.index)
	{
		return false;
	}
	for (size_t i = 0; i < Hole.Nodes.Num(); i++)
	{
		FTriangleNode c = Hole.Nodes[i];
		FTriangleNode d = Hole.Nodes[(i + 1) % Hole.Nodes.Num()];
		if (Detect2LineIntersect(node1, node2, c, d))
		{
			return true;
		}
	}
	return false;
}

bool UTriangleReplicationGraphNode_Triangle::DetectVisibility(FTriangle& T1, FTriangle& T2)
{
	FVector Center1 = FVector((T1.n1.x + T1.n2.x + T1.n3.x) / 3, (T1.n1.y + T1.n2.y + T1.n3.y) / 3, 200.0);
	FVector Center2 = FVector((T2.n1.x + T2.n2.x + T2.n3.x) / 3, (T2.n1.y + T2.n2.y + T2.n3.y) / 3, 200.0);
	const float DistSq = (Center1 - Center2).Size();
	if (DistSq > RelevantDistance)
	{
		return false;
	}
	for (FPolygon Hole : Holes)
	{
		if (DetectLineIntersectHole(T1.n1, T2.n1, Hole)
			&& DetectLineIntersectHole(T1.n1, T2.n2, Hole)
			&& DetectLineIntersectHole(T1.n1, T2.n3, Hole)
			&& DetectLineIntersectHole(T1.n2, T2.n1, Hole)
			&& DetectLineIntersectHole(T1.n2, T2.n2, Hole)
			&& DetectLineIntersectHole(T1.n2, T2.n3, Hole)
			&& DetectLineIntersectHole(T1.n3, T2.n1, Hole)
			&& DetectLineIntersectHole(T1.n3, T2.n2, Hole)
			&& DetectLineIntersectHole(T1.n3, T2.n3, Hole))
		{
			return false;
		}
	}
	return true;
}

void UTriangleReplicationGraphNode_Triangle::AddActorInternal_Dynamic(const FNewReplicatedActorInfo& ActorInfo)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (ActorInfo.Actor->bAlwaysRelevant)
	{
		UE_LOG(LogTriangleReplicationGraph, Warning, TEXT("Always relevant actor being added to spatialized graph node. %s"), *GetNameSafe(ActorInfo.Actor));
		return;
	}
#endif

	if (bNeedsRebuild)
	{
		bNeedsRebuild = false;
		BuildTriangles();
	}

	DynamicSpatializedActors.Emplace(ActorInfo.Actor, ActorInfo);
}


void UTriangleReplicationGraphNode_Triangle::RemoveActorInternal_Dynamic(const FNewReplicatedActorInfo& ActorInfo)
{
	if (FCachedDynamicActorInfo* DynamicActorInfo = DynamicSpatializedActors.Find(ActorInfo.Actor))
	{
		if (DynamicActorInfo->OwnerCell)
		{
			DynamicActorInfo->OwnerCell->RemoveDynamicActor(ActorInfo);
		}
		DynamicSpatializedActors.Remove(ActorInfo.Actor);
	}
	else
	{
		UE_LOG(LogTriangleReplicationGraph, Warning, TEXT("UReplicationGraphNode_Simple2DSpatialization::RemoveActorInternal_Dynamic attempted remove %s from streaming dynamic list but it was not there."), *GetActorRepListTypeDebugString(ActorInfo.Actor));
		// 		if (StaticSpatializedActors.Remove(ActorInfo.Actor) > 0)
		// 		{
		// 			UE_LOG(LogReplicationGraph, Warning, TEXT("   It was in StaticSpatializedActors!"));
		// 		}
	}
}

UTriangleReplicationGraphNode_TriangleCell* UTriangleReplicationGraphNode_Triangle::GetTriangleCellForLocation(const FVector& Location)
{
	for (UReplicationGraphNode* ChildNode : AllChildNodes)
	{
		UTriangleReplicationGraphNode_TriangleCell* TriangleCell = (UTriangleReplicationGraphNode_TriangleCell *)ChildNode;
		if (TriangleCell)
		{
			FTriangle Triangle = TriangleCell->Triangle;
			FVector A = FVector(Triangle.n1.x, Triangle.n1.y, 0.0);
			FVector B = FVector(Triangle.n2.x, Triangle.n2.y, 0.0);
			FVector C = FVector(Triangle.n3.x, Triangle.n3.y, 0.0);
			if (FGeomTools::PointInTriangle(A, B, C, Location, SMALL_NUMBER))
			{
				return TriangleCell;
			}
		}
	}
	return nullptr;
}

// UTriangleReplicationGraphNode_TriangleCell
UTriangleReplicationGraphNode_TriangleCell::UTriangleReplicationGraphNode_TriangleCell()
{
	bRequiresPrepareForReplicationCall = true;

}

void UTriangleReplicationGraphNode_TriangleCell::SetTriangle(FTriangle& OutTriangle)
{
	Triangle = OutTriangle;
	FTriangleNode n1 = Triangle.n1;
	FTriangleNode n2 = Triangle.n2;
	FTriangleNode n3 = Triangle.n3;

	FActorSpawnParameters SpawnInfo;

	FVector Center = FVector((n1.x + n2.x + n3.x) / 3, (n1.y + n2.y + n3.y) / 3, 200.0);
// 	ATriangleActor* TriangleActor = GetWorld()->SpawnActor<ATriangleActor>(Center, FRotator(0.0, 0.0, 0.0), SpawnInfo);
// 	TriangleActor->SetTriangle(OutTriangle);
}

UReplicationGraphNode* UTriangleReplicationGraphNode_TriangleCell::GetDynamicNode()
{
	if (DynamicNode == nullptr)
	{
		if (CreateDynamicNodeOverride)
		{
			DynamicNode = CreateDynamicNodeOverride(this);
		}
		else
		{
			DynamicNode = CreateChildNode<UReplicationGraphNode_ActorListFrequencyBuckets>();
		}
	}

	return DynamicNode;
}

void UTriangleReplicationGraphNode_TriangleCell::AddDynamicActor(const FNewReplicatedActorInfo& ActorInfo)
{
	GetDynamicNode()->NotifyAddNetworkActor(ActorInfo);
}

void UTriangleReplicationGraphNode_TriangleCell::RemoveDynamicActor(const FNewReplicatedActorInfo& ActorInfo)
{
	// 	UE_CLOG(CVar_RepGraph_LogActorRemove > 0, LogTriangleReplicationGraph, Display, TEXT("UReplicationGraphNode_Simple2DSpatializationLeaf::RemoveDynamicActor %s on %s"), *ActorInfo.Actor->GetPathName(), *GetPathName());

	GetDynamicNode()->NotifyRemoveNetworkActor(ActorInfo);
}

void UTriangleReplicationGraphNode_TriangleCell::GetAllActorsInNode_Debugging(TArray<FActorRepListType>& OutArray) const
{
	Super::GetAllActorsInNode_Debugging(OutArray);
}

#pragma optimize( "", on )