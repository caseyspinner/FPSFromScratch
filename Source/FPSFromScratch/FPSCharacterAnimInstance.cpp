// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSCharacterAnimInstance.h"
#include "GameFramework\Pawn.h"
#include "GameFramework\CharacterMovementComponent.h"
#include "FPSCharacter.h"

void UFPSCharacterAnimInstance::NativeInitializeAnimation()
{
	// First we initialize our animating pawn as our character
	if (Pawn == nullptr)
	{
		Pawn = TryGetPawnOwner();
		if (Pawn)
		{
			Character = Cast<AFPSCharacter>(Pawn);
		}
	}
}

void UFPSCharacterAnimInstance::UpdateAnimationProperties()
{
	if (Pawn == nullptr)
	{
		Pawn = TryGetPawnOwner();
	}

	if (Pawn)
	{
		FVector Speed = Pawn->GetVelocity();

		FVector LateralSpeed = FVector(Speed.X, Speed.Y, 0.f);

		MovementSpeed = LateralSpeed.Size(); // Gets magnitude of the speed component

		bIsInAir = Pawn->GetMovementComponent()->IsFalling();

		if (Character == nullptr)
		{
			Character = Cast<AFPSCharacter>(Pawn);
		}
	}
}
