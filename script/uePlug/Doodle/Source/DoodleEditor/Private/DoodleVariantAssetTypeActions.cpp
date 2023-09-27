// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleVariantAssetTypeActions.h"
#include "DoodleVariantCompoundWidget.h"

DoodleVariantAssetTypeActions::DoodleVariantAssetTypeActions()
{
}

DoodleVariantAssetTypeActions::DoodleVariantAssetTypeActions(EAssetTypeCategories::Type InAssetCategory)
{
	MyAssetCategory = InAssetCategory;
}

DoodleVariantAssetTypeActions::~DoodleVariantAssetTypeActions()
{
}

void DoodleVariantAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor) {
    const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

    
    for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt) 
    {
        TSharedPtr<SDockTab> t = FGlobalTabmanager::Get()->TryInvokeTab(DoodleVariantCompoundWidget::Name);
        TSharedRef<DoodleVariantCompoundWidget> sva = StaticCastSharedRef<DoodleVariantCompoundWidget>(t->GetContent());
        UDoodleVariantObject* obj = Cast<UDoodleVariantObject>(*ObjIt);
        sva->SetSetVariantData(obj);
    }
}