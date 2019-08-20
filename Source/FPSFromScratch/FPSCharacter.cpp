// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "MainPlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "FPSWeapon.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"

// Sets default values
AFPSCharacter::AFPSCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Setting our turn rates for input (controller, not mouse)
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	bMovingForward = false;
	bMovingRight = false;

	// These could be affected in the future by multipliers i.e. carry weight
	BaseWalkSpeed = 200.f;
	BaseRunSpeed = 650.f;

	// We will default to letting the player fire the weapon for now
	bCanStartFiringWeapon = true;

	bFireWeaponKeyIsPressed = false;

	// Creating the mesh component for our character
	CharacterMesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh3P"));
	CharacterMesh3P->SetupAttachment(GetCapsuleComponent());
	/*CharacterMesh3P->bOwnerNoSee = true;*/

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(CharacterMesh3P, FName("LeftEyeSocket"));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	/*CharacterMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	CharacterMesh1P->SetupAttachment(FirstPersonCameraComponent);
	CharacterMesh1P->bOnlyOwnerSee = true;*/

}

// Called when the game starts or when spawned
void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	MainPlayerController = Cast<AMainPlayerController>(GetController());

	if (SelectedWeapon)
	{
		EquipWeapon();

		UE_LOG(LogTemp, Warning, TEXT("Weapon Equipped in BeginPlay"));
	}
}

void AFPSCharacter::MoveForward(float value)
{
	// Since this is called in the tick function, it will be false
	// until we provide input.
	bMovingForward = false;

	if (value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), value);

		bMovingForward = true;
	}
}

void AFPSCharacter::MoveRight(float value)
{
	// Since this is called in the tick function, it will be false
	// until we provide input.
	bMovingRight = false;

	if (value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), value);

		bMovingRight = true;
	}
}

void AFPSCharacter::EquipWeapon()
{
	if (SelectedWeapon)
	{
		AFPSWeapon* Weapon = GetWorld()->SpawnActor<AFPSWeapon>(SelectedWeapon); 

		if (Weapon)
		{
			EquippedWeapon = Weapon;
			Weapon->InitWeaponToPlayer(this);
			UE_LOG(LogTemp, Warning, TEXT("Weapon Spawned"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Weapon NOT Spawned"));
		}
	}
}

void AFPSCharacter::StartFireWeapon()
{
	// We call our OnFire function in the weapon class
	if (EquippedWeapon)
	{
		AFPSWeapon* Weapon = Cast<AFPSWeapon>(EquippedWeapon);

		if (Weapon)
		{
			Weapon->HandleWeaponFire();
		}
	}
}

void AFPSCharacter::PlayAnimMontageAtSection(UAnimMontage* AnimMontage, float PlaybackRate, FName SectionName)
{
	UAnimInstance* AnimInstance = GetSkeletalMesh3P()->GetAnimInstance();

	if (AnimMontage && AnimInstance)
	{
		AnimInstance->Montage_Play(AnimMontage, PlaybackRate);
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AFPSCharacter::OnStartFireWeapon()
{
	bFireWeaponKeyIsPressed = true;

	StartFireWeapon();
}

void AFPSCharacter::OnFinishFireWeapon()
{
	bFireWeaponKeyIsPressed = false;

	GetWorldTimerManager().ClearTimer(EquippedWeapon->TimerHandle_HandleFiring);
}

// Called every frame
void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	// Bind first person camera control to mouse input
	PlayerInputComponent->BindAxis("CameraYaw", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("CameraPitch", this, &APawn::AddControllerPitchInput);

	// Bind movement events (keyboard)
	PlayerInputComponent->BindAxis("MoveForward", this, &AFPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFPSCharacter::MoveRight);

	// Bind weapon firing key
	PlayerInputComponent->BindAction("FireWeapon", IE_Pressed, this, &AFPSCharacter::OnStartFireWeapon);
	PlayerInputComponent->BindAction("FireWeapon", IE_Released, this, &AFPSCharacter::OnFinishFireWeapon);
}

