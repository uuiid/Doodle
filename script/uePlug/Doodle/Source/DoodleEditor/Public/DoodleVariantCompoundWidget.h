// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "DoodleVariantObject.h"

#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"
#include "ISequencer.h"

//struct ItemData
//{
//	FString Name;
//	FString NewName;
//	int Index = -1;
//};

struct FMaterialItemData
{
	FName Slot;
	TObjectPtr<UMaterialInterface> Material;
	int Index = -1;
}; 

/**
 * 
 */
class DOODLEEDITOR_API DoodleVariantCompoundWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(DoodleVariantCompoundWidget)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	FReply OnLoadAllVariant();
	FReply OnVariantAdd();
	FReply OnVariantAttach();

	//¹Ø¿¨ÐòÁÐ
	static ISequencer* TheSequencer;
	static void SequencerTrackObjectMenuBuilder(FMenuBuilder& builder);

	const static FName Name;
	static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

	///var
	TSharedPtr<SListView< TSharedPtr<FString>>> ThisListView;
	TArray<TSharedPtr<FString>> Items = {};
	//mate
	TSharedPtr<SListView<TSharedPtr<FMaterialItemData> >> MaterialListView;
	TArray< TSharedPtr< FMaterialItemData> > MaterialItems;
	//------------------------
	UDoodleVariantObject* MyObject;

	TSharedPtr<STextBlock> NameText;
	TSharedPtr<STextBlock> SelectText;

	void SetSetVariantData(UDoodleVariantObject* obj);
	void SetVariantInfo(FString varaint_name);

	/** Pool for maintaining and rendering thumbnails */
	TSharedPtr<FAssetThumbnailPool> AssetThumbnailPool = MakeShareable(new FAssetThumbnailPool(1024));
	FString NowVaraint;

	//----------------------
};
