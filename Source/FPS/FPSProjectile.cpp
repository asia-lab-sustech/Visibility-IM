// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "FPSProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "UnrealNetwork.h"
#include "ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AFPSProjectile::AFPSProjectile() 
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(20.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
// 	CollisionComp->BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->OnComponentHit.AddDynamic(this, &AFPSProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
// 	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
// 	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
// 	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->bShouldBounce = false;

	// Die after 3 seconds by default
// 	InitialLifeSpan = 3.0f;

	if (Role == ROLE_Authority)
	{
		SetReplicates(true);
	}

// 	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
// 	ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshLoader(_T("StaticMesh'/Game/FirstPerson/Meshes/FirstPersonProjectileMesh.FirstPersonProjectileMesh'"));
// 	UStaticMesh* StaticMesh = (UStaticMesh*)StaticMeshLoader.Object;
// 
// 	Mesh->SetStaticMesh(StaticMesh);
}

void AFPSProjectile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	DOREPLIFETIME(AFPSProjectile, Count);
}

void AFPSProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Role == ROLE_Authority)
	{
		GetWorldTimerManager().SetTimer(Timer, this, &AFPSProjectile::IncreaseCount, 1.0f, true, 1.0f);
	}
}

void AFPSProjectile::IncreaseCount()
{
	Count++;
}

void AFPSProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only add impulse and destroy projectile if we hit a physics
	if ((OtherActor != NULL) && (OtherActor != this) && (OtherComp != NULL) && OtherComp->IsSimulatingPhysics())
	{
		OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());

		Destroy();
	}
}