// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleVariantAssetTypeActions.h"
#include "DoodleVariantCompoundWidget.h"
#include "DoodleVariantEditorToolkit.h"

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
    //---------------
    for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt) 
    {
        //TSharedPtr<SDockTab> t = FGlobalTabmanager::Get()->TryInvokeTab(DoodleVariantCompoundWidget::Name);
        //TSharedRef<DoodleVariantCompoundWidget> VariantWidget = StaticCastSharedRef<DoodleVariantCompoundWidget>(t->GetContent());
        UDoodleVariantObject* Asset = Cast<UDoodleVariantObject>(*ObjIt);
        if (Asset)
        {
            TSharedRef<UDoodleVariantEditorToolkit> NewToolkit = MakeShareable(new UDoodleVariantEditorToolkit());
            NewToolkit->Initialize(Mode,EditWithinLevelEditor, Asset);
        }
    }
}