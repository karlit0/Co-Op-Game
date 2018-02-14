// Fill out your copyright notice in the Description page of Project Settings.

#include "SProjectileWeapon.h"




void ASProjectileWeapon::Fire()
{
	// Trace the world, from pawn eyes to crosshair location

	AActor* myOwner = GetOwner();
	if (myOwner && ProjectileClass)
	{
		FVector eyeLocation;
		FRotator eyeRotation;
		myOwner->GetActorEyesViewPoint(eyeLocation, eyeRotation);
		
		FVector muzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FActorSpawnParameters spawnParams;
		spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(ProjectileClass, muzzleLocation, eyeRotation, spawnParams);
	}
}
