// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ReplicationGraph.h"
#include "TriangleStruct.h"
#include "FPSProjectile.h"
#include "TriangleReplicationGraph.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogTriangleReplicationGraph, Display, All);


UCLASS()
class UTriangleReplicationGraphNode_TriangleCell : public UReplicationGraphNode_ActorList
{
	GENERATED_BODY()

public:
	UTriangleReplicationGraphNode_TriangleCell();
	virtual void NotifyAddNetworkActor(const FNewReplicatedActorInfo& ActorInfo) override { ensureMsgf(false, TEXT("UReplicationGraphNode_Simple2DSpatializationLeaf::NotifyAddNetworkActor not functional.")); }
	virtual bool NotifyRemoveNetworkActor(const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound = true) override { ensureMsgf(false, TEXT("UReplicationGraphNode_Simple2DSpatializationLeaf::NotifyRemoveNetworkActor not functional.")); return false; }
	virtual void GetAllActorsInNode_Debugging(TArray<FActorRepListType>& OutArray) const override;

	void SetTriangle(FTriangle& OutTriangle);

	FTriangle Triangle;

	// Allow graph to override function for creating the dynamic node in the cell
	TFunction<UReplicationGraphNode*(UTriangleReplicationGraphNode_TriangleCell* Parent)> CreateDynamicNodeOverride;

	UPROPERTY()
		UReplicationGraphNode* DynamicNode = nullptr;

	UReplicationGraphNode* GetDynamicNode();

	void AddDynamicActor(const FNewReplicatedActorInfo& ActorInfo);
	void RemoveDynamicActor(const FNewReplicatedActorInfo& ActorInfo);

	TArray<UTriangleReplicationGraphNode_TriangleCell*> RelevantCells;
};


UCLASS()
class UTriangleReplicationGraphNode_Triangle : public UReplicationGraphNode
{
	GENERATED_BODY()
public:
	UTriangleReplicationGraphNode_Triangle();

// 	virtual void NotifyAddNetworkActor(const FNewReplicatedActorInfo& Actor) override { }
	virtual void NotifyAddNetworkActor(const FNewReplicatedActorInfo& ActorInfo) override;
	virtual void NotifyResetAllNetworkActors() override;
	virtual bool NotifyRemoveNetworkActor(const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound = true) override;
	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params) override;
	virtual void PrepareForReplication() override;
	virtual void LogNode(FReplicationGraphDebugInfo& DebugInfo, const FString& NodeName) const override;

	void LoadTriangles();
	void AddNodeNeighbours(UTriangleReplicationGraphNode_TriangleCell * Cell1, uint32 NodeIndex);
	void BuildTriangles();
	bool DetectVisibility(FTriangle& T1, FTriangle& T2);

	struct FCachedDynamicActorInfo
	{
		FCachedDynamicActorInfo(const FNewReplicatedActorInfo& InInfo) : ActorInfo(InInfo) { }
		FNewReplicatedActorInfo ActorInfo;
		UTriangleReplicationGraphNode_TriangleCell* OwnerCell = nullptr;
	};

	TMap<FActorRepListType, FCachedDynamicActorInfo> DynamicSpatializedActors;

	void AddActor_Dynamic(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo) { AddActorInternal_Dynamic(ActorInfo); }
	virtual void AddActorInternal_Dynamic(const FNewReplicatedActorInfo& ActorInfo);

	void RemoveActor_Dynamic(const FNewReplicatedActorInfo& ActorInfo) { RemoveActorInternal_Dynamic(ActorInfo); }
	virtual void RemoveActorInternal_Dynamic(const FNewReplicatedActorInfo& Actor);

	UTriangleReplicationGraphNode_TriangleCell* GetTriangleCellForLocation(const FVector& Location);
private:

	TMap<uint32, TArray<UTriangleReplicationGraphNode_TriangleCell *>> Node2Triangles;
	TArray<FTriangleNode> Nodes;
	TArray<FTriangle> Triangles;
	TArray<FPolygon> Holes;

	bool bNeedsRebuild = true;
	bool bUseDistance = false;
	bool bUseNeighbour = true;
	

	int NeighbourDepth = 2;
	float RelevantDistance = 1500;

	double MatchingAverage = 0.0;
	int MatchingCount = 0;
};


UCLASS(transient, config = Engine)
class UTriangleReplicationGraph :public UReplicationGraph
{
	GENERATED_BODY()
public:
	UTriangleReplicationGraph();
	~UTriangleReplicationGraph();

	virtual void ResetGameWorldState() override;

	virtual void InitGlobalActorClassSettings() override;
	virtual void InitGlobalGraphNodes() override;
	virtual void InitConnectionGraphNodes(UNetReplicationGraphConnection* RepGraphConnection) override;
	virtual void RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo) override;
	virtual void RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo) override;

	UPROPERTY()
		UReplicationGraphNode_ActorList* AlwaysRelevantNode;

	UPROPERTY()
		UTriangleReplicationGraphNode_Triangle* TriangleNode;

};
