// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "FPSCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "MainPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Engine.h"
#include "Curves/CurveVector.h"

// Sets default values
AFPSWeapon::AFPSWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;

	// We will default to rifle for now
	WeaponType = EWeaponType::EWT_Rifle;

	// We will default to full-auto for now
	FiringMode = EWeaponFiringMode::EWFM_FullAuto;

	// We will choose default values for weapon stats for now
	BaseDamage = 25.f;
	MagazineCapacity = 30;
	LastFireTime = 0.0f;
	// 0.1 seconds between shots translates to 600 RPM
	TimeBetweenShots = 0.100f;
	StartingNumRoundsTotal = 120;

	bUnlimitedAmmo = true;

	RecoilRecoveryRate = 20.f;
	RateOfFireForgiveness = 0.0001f;

	bFinishedCompensatingRecoil = true;
	TotalRecoilAdded = FRotator(0.f, 0.f, 0.f);

	// The first shot base spread in radians
	FirstShotBaseSpread = 0.005f;
	MovementSpreadPenalty = 0.00025f;
	bWeaponHasNoSpread = true;

	// We start off at the beginning of our recoil pattern
	RecoilPatternTimeAccumulator = 0.f;
}

// Called when the game starts or when spawned
void AFPSWeapon::BeginPlay()
{
	Super::BeginPlay();

	// The weapon starts out full
	CurrentNumRoundsTotal = StartingNumRoundsTotal;
	CurrentNumRoundsInMag = MagazineCapacity;
	
}

// Called every frame
void AFPSWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Sets bFinishedCompensatingRecoil based on whether or not all recoil has been compensated for
	CheckIfRecoilFinishedCompensating();

	// Recovers Controller rotation which was offset by recoil
	RecoverRecoil(DeltaTime);

	// When we aren't shooting, we regress through our recoil pattern towards its starting point
	UpdateRecoilPatternTimeAccumulator(DeltaTime);
}

void AFPSWeapon::InitWeaponToPlayer(AFPSCharacter* Player)
{
	if (Player)
	{
		// Sets the character as the wielder of the weapon
		OwningPlayer = Player;

		// Sets the character as the instigator of damage with the weapon
		SetInstigator(Player->GetController());

		// This makes it so the character will not get zoomed in on if the weapon gets
		// between the character and the camera
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		// Ignore collision between the pawn and the weapon
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

		// Stop simulating physics so we can attach to the player
		WeaponMesh->SetSimulatePhysics(false);

		// Now we attach to the 3P mesh
		const USkeletalMeshSocket* RightHandSocket3P = Player->GetSkeletalMesh3P()->GetSocketByName("GunGripSocketR_Hand");

		if (RightHandSocket3P)
		{
			RightHandSocket3P->AttachActor(this, Player->GetSkeletalMesh3P());
			UE_LOG(LogTemp, Warning, TEXT("Weapon attached to socket"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Weapon NOT attached, socket not initialized."));
		}
	}
}

void AFPSWeapon::HandleWeaponFire()
{
	float CurrentGameTime = GetWorld()->GetTimeSeconds();

	if (OwningPlayer && OwningPlayer->GetFireWeaponKeyIsPressed() && WeaponCanFireAgain(CurrentGameTime, LastFireTime, TimeBetweenShots))
	{
		// Fires one shot if possible or reloads if ammo available
		OnFire();
	}
	else if (OwningPlayer->GetFireWeaponKeyIsPressed())
	{
		FString ElapsedTimeSinceLastShotString = FString::SanitizeFloat(CurrentGameTime - LastFireTime);

		UE_LOG(LogTemp, Warning, TEXT("ElapsedTimeSinceLastShot is %s"), *ElapsedTimeSinceLastShotString);
	}
}

void AFPSWeapon::OnFire()
{
	if (CurrentNumRoundsTotal > 0 && CurrentNumRoundsInMag > 0)
	{
		PlayFireWeaponEffects();
		StartWeaponTrace();

		if (!bUnlimitedAmmo)
		{
			ConsumeAmmo();
		}

		RecoilPatternTimeAccumulator = FMath::Clamp(RecoilPatternTimeAccumulator + TimeBetweenShots, 0.f, 3.f);
		
		AddRecoil();

		// Record the fact that we fired a shot via updating the LastFireTime
		LastFireTime = GetWorld()->GetTimeSeconds();

		/*if (OwningPlayer && OwningPlayer->GetFireWeaponKeyIsPressed())
		{*/
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AFPSWeapon::HandleWeaponFire, TimeBetweenShots);
		//}

	}
	else if (CurrentNumRoundsTotal > 0 && CurrentNumRoundsInMag == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Magazine empty but we have ammo in reserve!"));

		// If we have no ammo in the magazine, but ammo in reserve, we reload
		Reload();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot fire -- completely out of ammo!"));
	}
}

void AFPSWeapon::StartWeaponTrace()
{
	if (OwningPlayer)
	{
		AMainPlayerController* MainPlayerController = OwningPlayer->GetMainPlayerController();

		UCameraComponent* FirstPersonCameraComponent = OwningPlayer->GetCameraComponent1P();

		if (MainPlayerController && FirstPersonCameraComponent)
		{

			/**
			 * Our trace will start from the camera (eye-level), rather
			 * than from the apparent position of the weapon muzzle.
			 */
			FVector TraceStartLocation = FirstPersonCameraComponent->GetComponentLocation();

			/* We get the transform of the camera */
			FTransform CameraTransform = FirstPersonCameraComponent->GetComponentTransform();

			FQuat CameraQuat = CameraTransform.GetRotation();

			
			/* Our shots will land 2x the vector between our base rotation and the offset from recoil. To visualize this,
			 * if the player shoots at the wall without manually moving their crosshair, the bullet will land 2x the vector between their original crosshair position and their new crosshair position. To achieve this, we rotate our forward vector by the current value of TotalRecoilAdded. */

			if (!bFinishedCompensatingRecoil)
			{
				FRotator CameraAdjustedRotator = CameraQuat.Rotator() + TotalRecoilAdded;

				CameraQuat = FQuat(CameraAdjustedRotator);
			}

			// We add inaccuracy to our forward vector
			FVector ForwardVectorWithSpread = GetVectorAdjustedForSpread(CameraQuat.GetForwardVector());

			/**
			 * Now we will multiply the vector by some scalar to designate the length
			 * or distance that we want the vector to extend to.
			 */
			FVector TraceEndLocation = TraceStartLocation + ForwardVectorWithSpread * 3000;

			/**
			 * Now we do our trace from the start location to the end location.
			 * Our HitResult will contain the result of our trace and tell us
			 * if there was any blocking hit. Our CollisionQueryParams will
			 * give some params for the trace.
			 */
			FHitResult HitResult;
			FCollisionQueryParams CollisionQueryParams;
			FCollisionResponseParams CollisionResponseParams;

			UWorld* World = GetWorld();

			if (World)
			{
				World->LineTraceSingleByChannel(HitResult, TraceStartLocation, TraceEndLocation, ECC_Visibility, CollisionQueryParams, CollisionResponseParams);
			}

			FVector HitLocation = HitResult.Location;

			// Displays the hit location with a blue dot, and the trace start with a red
			// dot.
			ShowDebugHitLocation(TraceStartLocation, HitLocation);
		}
	}
}

void AFPSWeapon::PlayFireWeaponEffects()
{
	// This is the socket we will spawn the particles on and play the sound from
	const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMesh->GetSocketByName("Muzzle");

	if (MuzzleFlashSocket)
	{
		// The world location of our weapon muzzle socket
		FVector SocketLocation = MuzzleFlashSocket->GetSocketLocation(WeaponMesh);

		// Plays weapon fire sound
		if (FireSoundBase)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSoundBase, SocketLocation);
		}

		// Spawns muzzle flash
		if (MuzzleFlashParticles)
		{

			/* We will be scaling it down (by a hard coded value for now) */
			FVector MuzzleFlashScale(0.05f, 0.05f, 0.05f);

			/* The muzzle flash will be pointed in the same direction as the muzzle */
			FRotator MuzzleRotation = MuzzleFlashSocket->GetSocketTransform(WeaponMesh).Rotator();

			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashParticles, SocketLocation, MuzzleRotation, MuzzleFlashScale, true);

			// Later on we will attach the particles to the muzzle
			/*UGameplayStatics::SpawnEmitterAttached(MuzzleFlashParticles, MuzzleFlashSocket, FName("Muzzle"), FVector(0.f), MuzzleRotation, MuzzleFlashScale, EAttachLocation::KeepRelativeOffset, true, EPSCPoolMethod::None);*/

		}
	}

	// This will trigger our player to play the firing weapon animations
	if (OwningPlayer)
	{
		// Our player's firing weapon montage
		UAnimMontage* ShootingMontage = OwningPlayer->FiringWeaponMontage;
		
		if (ShootingMontage)
		{
			// We will hard code this for now because we know the montage length is 0.25 sec
			float MontagePlayRate = 0.25f / TimeBetweenShots;

			OwningPlayer->PlayAnimMontageAtSection(ShootingMontage, MontagePlayRate, FName("Firing_Standing"));
		}
	}
}

void AFPSWeapon::ConsumeAmmo()
{
	if (CurrentNumRoundsInMag > 0 && CurrentNumRoundsTotal > 0)
	{
		// If we have ammo in the magazine, we consume one round
		CurrentNumRoundsInMag--;
		CurrentNumRoundsTotal--;
	}
	else if (CurrentNumRoundsTotal > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("We need to reload!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Completely out of ammo!"));
	}
}

void AFPSWeapon::Reload()
{
	// CanReload() currently checks based on available ammo and nothing else
	if (CanReload())
	{
		// Number of rounds missing from magazine
		int32 NumRoundsBeforeFullMag = MagazineCapacity - CurrentNumRoundsInMag;

		// Num rounds available which are not in a magazine
		int32 NumRoundsInReserve = CurrentNumRoundsTotal - CurrentNumRoundsInMag;

		// We fill the magazine to full or with as many rounds as we have left
		int32 NumRoundsToReload = FMath::Min(NumRoundsBeforeFullMag, NumRoundsInReserve);

		CurrentNumRoundsInMag += NumRoundsToReload;

		UE_LOG(LogTemp, Warning, TEXT("Magazine Reloaded"));

		GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to reload"));
	}
}

bool AFPSWeapon::CanReload()
{
	return ((CurrentNumRoundsInMag < MagazineCapacity) && (CurrentNumRoundsTotal > CurrentNumRoundsInMag));
}

bool AFPSWeapon::WeaponCanFireAgain(float CurrentTime, float LastTimeFired, float MinTimeBetweenShots)
{
	return ((CurrentTime - LastTimeFired >= MinTimeBetweenShots) || (FMath::IsNearlyEqual((CurrentTime - LastFireTime), MinTimeBetweenShots, RateOfFireForgiveness)));
}

// On weapon fire
void AFPSWeapon::AddRecoil()
{
	AMainPlayerController* PlayerController = OwningPlayer->GetMainPlayerController();

	if (PlayerController->IsLocalPlayerController())
	{
		FRotator CurrentRotation = PlayerController->GetControlRotation();

		FRotator NewRotation = CurrentRotation + GetRecoilValueToAdd();

		FString RecoilValueToAddString(GetRecoilValueToAdd().ToString());

		uint64 RecoilValueToAddId = 777;

		GEngine->AddOnScreenDebugMessage(RecoilValueToAddId, 15.0f, FColor::Blue, *RecoilValueToAddString);

		TotalRecoilAdded += GetRecoilValueToAdd();

		PlayerController->SetControlRotation(NewRotation);
	}
}

// On Tick
void AFPSWeapon::CheckIfRecoilFinishedCompensating()
{
	if (TotalRecoilAdded.IsZero())
	{
		bFinishedCompensatingRecoil = true;
	}
	else
	{
		bFinishedCompensatingRecoil = false;
	}
}


// On Tick
void AFPSWeapon::RecoverRecoil(float DeltaTime)
{
	if (!bFinishedCompensatingRecoil)
	{
		AMainPlayerController* PlayerController = OwningPlayer->GetMainPlayerController();

		if (PlayerController->IsLocalPlayerController())
		{
			// Start with no added rotation
			FRotator NoRotation(0.f, 0.f, 0.f);

			FRotator CurrentRotation = PlayerController->GetControlRotation();

			// Use percentage of rotation done for number of shots fired from recoil pattern
			FRotator Compensation = FMath::RInterpConstantTo(NoRotation, TotalRecoilAdded, DeltaTime, RecoilRecoveryRate);



			/*FRotator Compensation = FMath::Lerp(NoRotation, TotalRecoilAdded, 0.1f);*/

			/*Alpha = Compensation.Pitch / TotalRecoilAdded.Pitch;*/

			PlayerController->SetControlRotation(CurrentRotation - Compensation);

			TotalRecoilAdded -= (Compensation);

			FString CompensationString(Compensation.ToString());

			uint64 CompensationId = 333;

			FString TotalRecoilAddedString(TotalRecoilAdded.ToString());

			uint64 TotalRecoilAddedId = 444;

			GEngine->AddOnScreenDebugMessage(CompensationId, 15.0f, FColor::Red, *CompensationString);

			GEngine->AddOnScreenDebugMessage(TotalRecoilAddedId, 15.0f, FColor::Red, *TotalRecoilAddedString);

			/*if (Alpha == 1.0f)
			{
				TotalRecoilAdded = FRotator(0.f, 0.f, 0.f);
			}*/
		}
		
	}
	else if (bFinishedCompensatingRecoil)
	{
		TotalRecoilAdded = FRotator(0.f, 0.f, 0.f);
	}
}

FRotator AFPSWeapon::GetRecoilValueToAdd()
{
	if (RecoilPatternCurve)
	{
		UCurveVector* RecoilCurve = Cast<UCurveVector>(RecoilPatternCurve);

		if (RecoilCurve)
		{
			// This is how "far we are" through our recoil pattern as a float between 0 and 1
			float RecoilPatternPercentFloat = RecoilPatternTimeAccumulator / (MagazineCapacity * TimeBetweenShots);

			FString RecoilPatternPercentFloatString = FString::SanitizeFloat(RecoilPatternPercentFloat);

			uint64 RecoilPatternPercentFloatId = 666;

			GEngine->AddOnScreenDebugMessage(RecoilPatternPercentFloatId, 15.0f, FColor::Green, *RecoilPatternPercentFloatString);

			FVector RecoilVector = RecoilCurve->GetVectorValue(RecoilPatternPercentFloat);

			// To get the recoil value at a time (or percentage) between 0-1 we use GetVectorValue
			//return (RecoilCurve->GetVectorValue(RecoilPatternPercentFloat)).Rotation();
			return FRotator(RecoilVector.Y, RecoilVector.X, 0.f);
		}
	}

	return FRotator(0.f, 0.f, 0.f);
}

void AFPSWeapon::UpdateRecoilPatternTimeAccumulator(float DeltaTime)
{
	if (OwningPlayer && !(OwningPlayer->bFireWeaponKeyIsPressed))
	{
		RecoilPatternTimeAccumulator = FMath::Clamp(RecoilPatternTimeAccumulator - (TimeBetweenShots * TimeBetweenShots), 0.f, 3.f);
	}
}

FVector AFPSWeapon::GetVectorAdjustedForSpread(FVector InVector)
{
	if (bWeaponHasNoSpread)
	{
		return InVector;
	}

	float EffectiveSpread = FirstShotBaseSpread;

	if (OwningPlayer)
	{
		// Adds spread penalty based on the player's movement velocity.
		float PlayerVelocity = OwningPlayer->GetVelocity().Size();

		FString PlayerVelocityString = FString::SanitizeFloat(PlayerVelocity);

		uint64 PlayerVelocityStringId = 888;

		GEngine->AddOnScreenDebugMessage(PlayerVelocityStringId, 15.0f, FColor::Yellow, *PlayerVelocityString);

		EffectiveSpread += PlayerVelocity * MovementSpreadPenalty;
	}

	return FMath::VRandCone(InVector, EffectiveSpread);
}

void AFPSWeapon::ShowDebugHitLocation(FVector StartLocation, FVector EndLocation)
{
	if (OwningPlayer)
	{
		UKismetSystemLibrary::DrawDebugPoint(this, StartLocation, 5.f, FLinearColor::Red, 5.f);

		UKismetSystemLibrary::DrawDebugPoint(this, EndLocation, 5.f, FLinearColor::Blue, 5.f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No owning player!"));
	}
}

