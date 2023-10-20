// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "DoodleVariantObject.h"

#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"

DECLARE_DELEGATE_OneParam(FVariantInfoParamDelegate, FVariantInfo);
DECLARE_DELEGATE_TwoParams(FECheckBoxStateIntParamDelegate, ECheckBoxState,int);
DECLARE_DELEGATE_OneParam(FIntParamDelegate, int);
DECLARE_DELEGATE_RetVal_OneParam(ECheckBoxState, FIntReturnECheckBoxStateParam,int);

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
	void Construct(const FArguments& InArgs);

private:
	TSharedRef<ITableRow> VariantListOnGenerateRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> MaterialListOnGenerateRow(TSharedPtr<FMaterialItemData> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void VariantNameOnTextCommitted(const FText& InText,TSharedPtr<FString> InItem);
	//-----------------------
	FReply OnLinkSkeletalMesh();
	FReply OnLoadAllVariant();
	FReply OnVariantAdd();
	void OnVariantDelete();

	static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

	TSharedPtr<SListView< TSharedPtr<FString>>> ThisListView;
	TArray<TSharedPtr<FString>> Items = {};
	//mate
	TSharedPtr<SListView<TSharedPtr<FMaterialItemData> >> MaterialListView;
	TArray< TSharedPtr< FMaterialItemData> > MaterialItems;
	//------------------------
	TSharedPtr<STextBlock> NameText;

	void SetVariantInfo(FString varaint_name);

	TSharedPtr<SButton> ButtonLoadVariant;
	TSharedPtr<SButton> ButtonLinkMesh;

public:
	FString NowVaraint;
	const static FName Name;
	void SetSetVariantData(UDoodleVariantObject* obj);

	UDoodleVariantObject* CurrentObject;
	//----------------------
	FVariantInfoParamDelegate OnVariantChange;
	FIntParamDelegate IsIsolateMaterialEnabled;
	FECheckBoxStateIntParamDelegate OnMaterialCheckStateChanged;
	FIntReturnECheckBoxStateParam MaterialGetCheckState;
};
