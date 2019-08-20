// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSCharacter.generated.h"

UCLASS()
class FPSFROMSCRATCH_API AFPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	// Sets default values for this character's properties
	AFPSCharacter();

	/* Main Character mesh for 3rd person (what other players see) */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Mesh)
	class USkeletalMeshComponent* CharacterMesh3P;
	
	/* Main Character mesh for first person (only seen by self) */
	/*UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Mesh)
	class USkeletalMeshComponent* CharacterMesh1P;*/

	/* First person camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Controller")
	class AMainPlayerController* MainPlayerController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon)
	class AFPSWeapon* EquippedWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon)
	TSubclassOf<AFPSWeapon> SelectedWeapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	class UAnimMontage* FiringWeaponMontage;

	/* Camera turn rate (joystick). Could be affected by sensitivity settings in the future */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseTurnRate;

	/* Camera look up/down rate (joystick). Could be affected by sensitivity settings in the future */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseLookUpRate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MovementStatus")
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MovementStatus")
	float BaseRunSpeed;

	// This tells us if we are moving forward
	bool bMovingForward;

	// Tells us if we are moving right (or left)
	bool bMovingRight;

	// Tells us if we can initiate firing the weapon
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	bool bCanStartFiringWeapon;

	// Tells us if the weapon firing key is down
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	bool bFireWeaponKeyIsPressed;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	//////////// State Management (booleans) //////////

	FORCEINLINE bool GetFireWeaponKeyIsPressed() { return bFireWeaponKeyIsPressed; }

	/////////////// Input Handling //////////////////

	// Handles forward and backward movement
	void MoveForward(float value);

	// Handles right and left movement
	void MoveRight(float value);

	// When fire weapon key is pressed
	void OnStartFireWeapon();

	// When fire weapon key is released
	void OnFinishFireWeapon();

	
	/////////////// Weapon Usage ////////////////////

	// Initializes the weapon to the player
	void EquipWeapon();

	void StartFireWeapon();

	///////////// Animation Functions ///////////////////

	// Plays an anim montage at a specified section name
	void PlayAnimMontageAtSection(UAnimMontage* AnimMontage, float PlaybackRate, FName SectionName);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	FORCEINLINE USkeletalMeshComponent* GetSkeletalMesh3P() const { return CharacterMesh3P; }
	
	/*FORCEINLINE USkeletalMeshComponent* GetSkeletalMesh1P() const { return CharacterMesh1P; }*/

	UFUNCTION(BlueprintCallable)
	FORCEINLINE UCameraComponent* GetCameraComponent1P() const { return FirstPersonCameraComponent; }

	FORCEINLINE AMainPlayerController* GetMainPlayerController() const { return MainPlayerController; }
};
