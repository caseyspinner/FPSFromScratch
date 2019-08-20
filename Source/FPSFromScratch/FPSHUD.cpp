// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "CanvasItem.h"
#include "UObject/ConstructorHelpers.h"

AFPSHUD::AFPSHUD()
{
	// Set the crosshair texture
	static ConstructorHelpers::FObjectFinder<UTexture2D> CrosshairTextureObj(TEXT("/Game/FirstPerson/Textures/crosshair007.crosshair007"));

	CrosshairTexture = CrosshairTextureObj.Object;

	CrosshairSizeMultiplier = 0.5f;
}

void AFPSHUD::DrawHUD()
{
	// We will draw our crosshair in the center of the canvas/screen

	// Find center of the canvas
	const FVector2D CenterPoint(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);

	// Offset by half the texture's dimensions so that the center of the texture aligns with the center of the Canvas
	uint32 CrosshairBaseWidth = CrosshairTexture->GetSizeX();
	uint32 CrosshairBaseHeight = CrosshairTexture->GetSizeY();

	// In the future we can scale the user's crosshair to their liking
	// but for now the scale is hard coded in the constructor
	float CrosshairEffectiveWidth = CrosshairBaseWidth * CrosshairSizeMultiplier;
	float CrosshairEffectiveHeight = CrosshairBaseHeight * CrosshairSizeMultiplier;

	const FVector2D CrosshairDrawPosition((CenterPoint.X - CrosshairEffectiveWidth /2),
		(CenterPoint.Y - CrosshairEffectiveHeight /2));

	const FVector2D CrosshairSize(CrosshairEffectiveWidth, CrosshairEffectiveHeight);

	// Draw Crosshair
	FCanvasTileItem TileItem(CrosshairDrawPosition, CrosshairTexture->Resource, CrosshairSize, FLinearColor::Green);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);
}
