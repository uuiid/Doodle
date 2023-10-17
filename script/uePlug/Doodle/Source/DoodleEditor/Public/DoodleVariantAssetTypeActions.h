// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DoodleVariantObject.h"
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"


/**
 * 
 */
class DOODLEEDITOR_API DoodleVariantAssetTypeActions : public FAssetTypeActions_Base
{
public:
	DoodleVariantAssetTypeActions();
	DoodleVariantAssetTypeActions(EAssetTypeCategories::Type InAssetCategory);
	~DoodleVariantAssetTypeActions();
protected:
	virtual FText GetName() const override { return FText::FromString("Doodle Variant"); };
	virtual FColor GetTypeColor() const override { return FColor(100, 100, 100); };
	virtual UClass* GetSupportedClass() const override { return  UDoodleVariantObject::StaticClass(); };
	virtual uint32 GetCategories() override { return MyAssetCategory; };

	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
private:
	EAssetTypeCategories::Type MyAssetCategory;
};
