// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CoOpGame.h"
#include "TimerManager.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing (
	TEXT("COOP.DebugWeapons"),
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat);

// Sets default values
ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";

	BaseDamage = 20.f;

	RateOfFire = 600;

	SetReplicates(true);
}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60.f / RateOfFire;
}

void ASWeapon::Fire()
{
	// Trace the world, from pawn eyes to crosshair location

	if (Role < ROLE_Authority)
	{
		ServerFire();
	}

	AActor* myOwner = GetOwner();
	if (myOwner)
	{
		FVector eyeLocation;
		FRotator eyeRotation;
		myOwner->GetActorEyesViewPoint(eyeLocation, eyeRotation);

		FVector shotDirection = eyeRotation.Vector();
		FVector traceEnd = eyeLocation + (shotDirection * 10000);

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(myOwner);
		queryParams.AddIgnoredActor(this);
		queryParams.bTraceComplex = true;
		queryParams.bReturnPhysicalMaterial = true;

		// Particle "Target" parameter
		FVector tracerEndPoint = traceEnd;

		FHitResult hit;
		if (GetWorld()->LineTraceSingleByChannel(hit, eyeLocation, traceEnd, COLLISION_WEAPON, queryParams))
		{
			// Blocking hit! Process damage
			AActor* hitActor = hit.GetActor();


			EPhysicalSurface surfaceType = UPhysicalMaterial::DetermineSurfaceType(hit.PhysMaterial.Get());

			float actualDamage = BaseDamage;
			if (surfaceType == SURFACE_FLESHVULNERABLE)
			{
				actualDamage *= 4.0f;
			}

			UGameplayStatics::ApplyPointDamage(hitActor, actualDamage, shotDirection, hit, myOwner->GetInstigatorController(), this, DamageType);

			UParticleSystem* selectedEffect = nullptr;
			switch (surfaceType)
			{
				case SURFACE_FLESHDEFAULT:
				case SURFACE_FLESHVULNERABLE:
					selectedEffect = FleshImpactEffect;
					break;
				default:
					selectedEffect = DefaultImpactEffect;
					break;
			}

			if (selectedEffect)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), selectedEffect, hit.ImpactPoint, hit.ImpactNormal.Rotation());
			}

			tracerEndPoint = hit.ImpactPoint;
		}

		if (DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), eyeLocation, traceEnd, FColor::White, false, 1.0f, 0, 1.0f);
		}

		PlayFireEffects(tracerEndPoint);

		LastFireTime = GetWorld()->TimeSeconds;
	}
}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::StartFire()
{
	float firstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, firstDelay);
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void ASWeapon::PlayFireEffects(FVector TraceEnd)
{
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	if (TracerEffect)
	{
		FVector muzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* tracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, muzzleLocation);
		if (tracerComp)
		{
			tracerComp->SetVectorParameter(TracerTargetName, TraceEnd);
		}
	}

	APawn* myOwner = Cast<APawn>(GetOwner());
	if (myOwner)
	{
		APlayerController* PC = Cast<APlayerController>(myOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
}
