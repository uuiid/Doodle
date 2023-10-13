// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleVariantEditorToolkit.h"
#include "DoodleVariantCompoundWidget.h"


void UDoodleVariantEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& In_TabManager) {
    FAssetEditorToolkit::RegisterTabSpawners(In_TabManager);
    In_TabManager->RegisterTabSpawner(
        UDoodleVariantEditorToolkit::ViewportID,
        FOnSpawnTab::CreateLambda([&](const FSpawnTabArgs& Args)
            {
                return SNew(SDockTab)
                    .Label(FText::FromString(TEXT("SpawnTab_Viewport")))
                    [
                        SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            [
                                SAssignNew(VariantEditorWidget, DoodleVariantCompoundWidget)
                            ]
                    ];
            })
    );
}

void UDoodleVariantEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& In_TabManager) {
    FAssetEditorToolkit::UnregisterTabSpawners(In_TabManager);
    In_TabManager->UnregisterTabSpawner(UDoodleVariantEditorToolkit::ViewportID);
}

FName UDoodleVariantEditorToolkit::GetToolkitFName() const
{
	return FName(TEXT(""));
}

FText UDoodleVariantEditorToolkit::GetBaseToolkitName() const
{
	return FText::FromString(TEXT(""));
}

FString UDoodleVariantEditorToolkit::GetWorldCentricTabPrefix() const
{
	return FString(TEXT(""));
}

FLinearColor UDoodleVariantEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0,0.5,0.5,0.5);
}

void UDoodleVariantEditorToolkit::Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UDoodleVariantObject* Asset)
{
    const TSharedRef<FTabManager::FLayout> L_Layout =
        FTabManager::NewLayout(
            "UDoodleVariantEditorToolkit_Initialize_Layout"
        )
        ->AddArea(
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Vertical)
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(3.f)
                ->SetHideTabWell(true)
                ->AddTab(UDoodleVariantEditorToolkit::ViewportID, ETabState::OpenedTab)
            )
        );
    InitAssetEditor(Mode, InitToolkitHost, AppIdentifier, L_Layout, true, true, Asset);
    if(VariantEditorWidget)
    VariantEditorWidget->SetSetVariantData(Asset);
}