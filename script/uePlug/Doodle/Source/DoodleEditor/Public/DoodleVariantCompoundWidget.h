// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "DoodleVariantObject.h"

#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"

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

	TSharedRef<ITableRow> VariantListOnGenerateRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> MaterialListOnGenerateRow(TSharedPtr<FMaterialItemData> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void VariantNameOnTextCommitted(const FText& InText,TSharedPtr<FString> InItem);
	//-----------------------
	FReply OnLoadAllVariant();
	FReply OnVariantAdd();
	void OnVariantDelete();
	FReply OnVariantAttach();

	const static FName Name;
	static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

	///var
	TSharedPtr<SListView< TSharedPtr<FString>>> ThisListView;
	TArray<TSharedPtr<FString>> Items = {};
	//mate
	TSharedPtr<SListView<TSharedPtr<FMaterialItemData> >> MaterialListView;
	TArray< TSharedPtr< FMaterialItemData> > MaterialItems;
	//------------------------
	UDoodleVariantObject* CurrentObject;

	TSharedPtr<STextBlock> NameText;
	TSharedPtr<STextBlock> SelectText;

	void SetSetVariantData(UDoodleVariantObject* obj);
	void SetVariantInfo(FString varaint_name);


	FString NowVaraint;
	//----------------------
};
