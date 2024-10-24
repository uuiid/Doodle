// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleVariantEditorToolkit.h"
#include "DoodleVariantCompoundWidget.h"
#include "DoodleVariantEditorViewport.h"
#include "Engine/SkinnedAssetCommon.h"

void UDoodleVariantEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& In_TabManager) 
{
    FAssetEditorToolkit::RegisterTabSpawners(In_TabManager);
    In_TabManager->RegisterTabSpawner(
        UDoodleVariantEditorToolkit::Variant,FOnSpawnTab::CreateLambda([&](const FSpawnTabArgs& Args)
            {
                return SNew(SDockTab)
                    .Label(FText::FromString(TEXT("Variant")))
                    [
                        SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            [
                                SAssignNew(VariantEditorWidget, DoodleVariantCompoundWidget)
                            ]
                    ];
            })
    );
    In_TabManager
        ->RegisterTabSpawner(
            UDoodleVariantEditorToolkit::Viewport, FOnSpawnTab::CreateLambda([&](const FSpawnTabArgs& Args)
                {
                    return SNew(SDockTab)
                        .Label(FText::FromString(TEXT("Viewport")))
                        [
                            SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                [
                                    SAssignNew(ViewEditorViewport, DoodleVariantEditorViewport)
                                ]
                        ];
                })
        );
}

void UDoodleVariantEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& In_TabManager) 
{
    In_TabManager->UnregisterTabSpawner(UDoodleVariantEditorToolkit::Viewport);
    In_TabManager->UnregisterTabSpawner(UDoodleVariantEditorToolkit::Variant);
    FAssetEditorToolkit::UnregisterTabSpawners(In_TabManager);
}

FName UDoodleVariantEditorToolkit::GetToolkitFName() const
{
	return FName(TEXT("ToolkitFName"));
}

FText UDoodleVariantEditorToolkit::GetBaseToolkitName() const
{
	return FText::FromString(TEXT("BaseToolkitName"));
}

FString UDoodleVariantEditorToolkit::GetWorldCentricTabPrefix() const
{
	return FString(TEXT("WorldCentricTabPrefix"));
}

FLinearColor UDoodleVariantEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0,0.5,0.5,0.5);
}

void UDoodleVariantEditorToolkit::Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UDoodleVariantObject* Asset)
{
    const TSharedRef<FTabManager::FLayout> L_Layout =
        FTabManager::NewLayout(
            "DoodleVariantEditorToolkit_Initialize_Layout"
        )
        ->AddArea(
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Horizontal)
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(3.f)
                ->SetHideTabWell(true)
                ->AddTab(UDoodleVariantEditorToolkit::Variant, ETabState::OpenedTab)
            )
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(3.f)
                ->SetHideTabWell(true)
                ->AddTab(UDoodleVariantEditorToolkit::Viewport, ETabState::OpenedTab)
            )
        );
    //--------------
    InitAssetEditor(Mode, InitToolkitHost, AppIdentifier, L_Layout, true, true, Asset);
    //-----------------
    if (ViewEditorViewport&& VariantEditorWidget)
    {
        VariantEditorWidget->SetSetVariantData(Asset);
        FString NowVaraint = VariantEditorWidget->NowVaraint;
        //-------------
        for (TSharedPtr<FString> Item : VariantEditorWidget->Items)
        {
            if (*Item == NowVaraint)
            {
                VariantEditorWidget->ThisListView->SetItemSelection(Item, true);
                break;
            }
        }
        ViewEditorViewport->SetViewportSkeletal(Asset->Mesh, VariantEditorWidget.Get()->CurrentObject->AllVaraint[NowVaraint].Variants);
        VariantEditorWidget->OnVariantChange.BindLambda([this,Asset](FVariantInfo variant)
        {
            ViewEditorViewport->SetViewportSkeletal(Asset->Mesh, variant.Variants);
        });
        VariantEditorWidget->MaterialGetCheckState.BindSP(ViewEditorViewport.Get(), &DoodleVariantEditorViewport::IsIsolateMaterialEnabled);
        VariantEditorWidget->OnMaterialCheckStateChanged.BindSP(ViewEditorViewport.Get(), &DoodleVariantEditorViewport::OnMaterialIsolatedChanged);
    }
   
}
