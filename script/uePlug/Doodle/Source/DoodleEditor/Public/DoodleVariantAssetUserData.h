// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "DoodleVariantObject.h"

#include "AssetRegistry/AssetDataTagMap.h"
#include "DoodleVariantAssetUserData.generated.h"

/**
 * 
 */
UCLASS()
class DOODLEEDITOR_API UDoodleVariantAssetUserData : public UAssetUserData
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnyWhere, Category = "Custom Data ")
	//UDoodleVariantObject* CustomData;

	//FAssetData& variant_asset{ *FAssetData };
	UDoodleVariantObject* variant_obj;
};
