// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSWeapon.generated.h"

/*
*/
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_Rifle	UMETA(DisplayName = "Rifle"),
	EWT_Pistol	UMETA(DisplayName = "Pistol"),

	EWT_MAX		UMETA(DisplayName = "DefaultMAX")
};

/*
*/
UENUM(BlueprintType)
enum class EWeaponFiringMode : uint8
{
	EWFM_SemiAuto	UMETA(DisplayName = "Semi Automatic"),
	EWFM_FullAuto	UMETA(DisplayName = "Fully Automatic"),

	EWFM_MAX		UMETA(DisplayName = "DefaultMAX")
};

/**
 * 
 */
UCLASS()
class FPSFROMSCRATCH_API AFPSWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFPSWeapon();

	/* Weapon mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Mesh)
	class USkeletalMeshComponent* WeaponMesh;

	/* Sound that plays when the weapon is fired */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class USoundBase* FireSoundBase;

	/* Particles which spawn at the muzzle location when the weapon is fired */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UParticleSystem* MuzzleFlashParticles;

	/* Designates the type of weapon. Used for determining 
	* animation type and more in the future.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay)
	EWeaponType WeaponType;

	/* Designates the firing mode of weapon. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay)
	EWeaponFiringMode FiringMode;

	/* Base damage per shot of the weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay)
	float BaseDamage;

	/* Max number of rounds per magazine */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay | Ammo")
	uint8 MagazineCapacity;

	/* Time the weapon was last fired. Used for making sure
	 * the weapon fires at an accurate fire rate, and will
	 * be used in weapon spread & recoil functionalities */
	float LastFireTime;

	/* Fire rate expressed in Period as seconds. f=1/T */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay)
	float TimeBetweenShots;

	/* Max number of rounds the weapon holds and starts with */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay | Ammo")
	uint8 StartingNumRoundsTotal;

	/* Current number of rounds left total */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Gameplay | Ammo")
	uint8 CurrentNumRoundsTotal;

	/* Current number of rounds left in magazine */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Gameplay | Ammo")
	uint8 CurrentNumRoundsInMag;

	// We will use the player controller for applying damage
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	AController* WeaponInstigator;

	// The player wielding the weapon
	class AFPSCharacter* OwningPlayer;

	// Timer handle to handle the firing of the weapon
	FTimerHandle TimerHandle_HandleFiring;

	// Unlimited ammo cheat for debug
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay | Cheats")
	bool bUnlimitedAmmo;

	/* When the weapon is fired, this is set to false, and set to true again once enough time has elapsed to satisfy the set fire rate. */
	bool bWeaponCanFire;

	/* The rate at which the weapon/camera recovers from recoil */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float RecoilRecoveryRate;

	/* This is the degree of tolerance given between timed shots due to the fact that timers in UE4 are not 100% accurate. The greater the value, the less-consistent our rate of fire is, the lower the value, the more likely our full-auto is to malfunction and not allow shooting. */
	float RateOfFireForgiveness;

	FORCEINLINE void SetInstigator(AController* Inst) { WeaponInstigator = Inst; }

	/* How we tell if our weapon is recoiling and recoil recovery is still needed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bFinishedCompensatingRecoil;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	class UCurveVector* RecoilPatternCurve;

	/* This records the rotation that has been added by recoil. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	FRotator TotalRecoilAdded;

	/* Every time we shoot, this is incremented by our fire rate. When we stop shooting, this is decremented. This is used to calculate where we are in our recoil pattern. */
	float RecoilPatternTimeAccumulator;

	///////// WEAPON ACCURACY //////////////////////

	/* Half angle of the cone encompassing the weapon spread on first shot. The first shot will land somewhere within the cone of this half-angle in radians.  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay | Accuracy")
	float FirstShotBaseSpread;

	/* The accuracy penalty given when the player is moving */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay | Accuracy")
	float MovementSpreadPenalty;

	/* Cheat for no spread */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay | Cheats")
	bool bWeaponHasNoSpread;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void InitWeaponToPlayer(AFPSCharacter* Player);
	
	///////////// WEAPON FIRING /////////////////////////

	/* Called on input from the owning player. Handles weapon firing. */
	void HandleWeaponFire();

	/* Fires one shot if available, otherwise reloads */
	void OnFire();

	/**
	 * Initiates a "hit scan" trace from the owning player's
	 * first-person camera component to the first blocking 
	 * hit. The hit will land in the center of the crosshair. 
	 */
	void StartWeaponTrace();
	
	/* This handles the visual and audio effects 
	 * which accompany the action of firing the weapon,
	 * such as muzzle flash, gunfire sound, etc. */
	void PlayFireWeaponEffects();

	////////////////// AMMO /////////////////////////////

	/* This will consume ammo when a shot is fired */
	void ConsumeAmmo();

	/* This reloads our gun, currently in the same way for all scenarios */
	void Reload();

	/* Determines if we can reload, based only on ammo count */
	bool CanReload();

	/* Uses time between shots to determine if the weapon can fire again and stick to the designated fire rate */
	bool WeaponCanFireAgain(float CurrentTime, float LastTimeFired, float MinTimeBetweenShots);

	//////////////// WEAPON RECOIL ////////////////////////

	/// On shot
	void AddRecoil();

	// On Tick
	void CheckIfRecoilFinishedCompensating();

	// On Tick
	void RecoverRecoil(float DeltaTime);

	/* Gets the recoil to be added for the next shot from our recoil curve */
	FRotator GetRecoilValueToAdd();

	/* Updates our position in the recoil pattern based on whether or not we are shooting. */
	void UpdateRecoilPatternTimeAccumulator(float DeltaTime);

	///////////////// WEAPON ACCURACY /////////////////////////////

	/* Returns our camera forward vector adjusted for weapon inaccuracy */
	FVector GetVectorAdjustedForSpread(FVector InVector);

	/**
	 * This is mainly for showing how traces function for the firing of
	 * weapons -- the start location of the ray will display a red dot
	 * and the first blocking hit of the trace will be shown as a blue dot.
	 */
	void ShowDebugHitLocation(FVector StartLocation, FVector EndLocation);

};
