// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FPSHUD.generated.h"

/**
 * 
 */
UCLASS()
class FPSFROMSCRATCH_API AFPSHUD : public AHUD
{
	GENERATED_BODY()
	
public:

	AFPSHUD();

	float CrosshairSizeMultiplier;

	/* Draws the HUD */
	virtual void DrawHUD() override;

private:

	/* Crosshair texture */
	class UTexture2D* CrosshairTexture;
};
