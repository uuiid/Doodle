// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ISequencer.h"

/**
 * 
 */
class DOODLEEDITOR_API DoodleVariantMenuExtension
{
public:
	DoodleVariantMenuExtension();
	~DoodleVariantMenuExtension();

	void AddMenuEntry(FMenuBuilder& MenuBuilder);
	void AddNewMenu(FMenuBuilder& builder, UDoodleVariantAssetUserData* UserData, AActor* TempActor);

	TWeakPtr<ISequencer> TheSequencer;
};
