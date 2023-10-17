// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DoodleVariantObject.h"
#include "DoodleVariantCompoundWidget.h"
#include "DoodleVariantEditorViewport.h"

/**
 * 
 */
class DOODLEEDITOR_API UDoodleVariantEditorToolkit : public FAssetEditorToolkit
{
protected:
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& In_TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& In_TabManager) override;

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
public:
	void Initialize(const EToolkitMode::Type Mode,const TSharedPtr<IToolkitHost>& InitToolkitHost,UDoodleVariantObject* Asset);
private:
	FName Variant{ TEXT("Doodle_Variant") };
	FName Viewport{ TEXT("Doodle_Viewport") };

	FName AppIdentifier{ TEXT("Doodle_AppIdentifier") };
	TSharedPtr<DoodleVariantCompoundWidget> VariantEditorWidget;

	TSharedPtr<DoodleVariantEditorViewport> ViewEditorViewport;
};
