// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ISequencer.h"

#include "DoodleVariantAssetUserData.h"

/**
 * 
 */
class DOODLEEDITOR_API FDoodleVariantMenuExtension
{
public:
	FDoodleVariantMenuExtension();
	~FDoodleVariantMenuExtension();

	void AddMenuEntry(FMenuBuilder& MenuBuilder);

private:
	void AddNewMenu(FMenuBuilder& builder, UDoodleVariantAssetUserData* UserData, AActor* TempActor);
public:
	TWeakPtr<ISequencer> TheSequencer;
};
