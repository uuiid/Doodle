// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "AdvancedPreviewScene.h"
#include "SCommonEditorViewportToolbarBase.h"


class DoodleVariantEditorViewport;

class DoodleVariantEditorPreviewScene : public FAdvancedPreviewScene {
public:
	DoodleVariantEditorPreviewScene();
	// FAdvancedPreviewScene 
	virtual void Tick(float InDeltaTime) override;
};

class DoodleVariantEditorViewportClient : public FEditorViewportClient {
public:
	DoodleVariantEditorViewportClient(FAssetEditorModeManager* InAssetEditorModeManager, FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget);

	virtual void Draw(FViewport* InViewport, FCanvas* Canvas) override;

	virtual void Tick(float DeltaSeconds) override;
};

class DoodleVariantEditorViewportToolBar : public SCommonEditorViewportToolbarBase {
public:
	SLATE_BEGIN_ARGS(DoodleVariantEditorViewportToolBar) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<DoodleVariantEditorViewport> InRealViewport);
};

/**
 * 
 */
class DOODLEEDITOR_API DoodleVariantEditorViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{
public:
	DoodleVariantEditorViewport();
	~DoodleVariantEditorViewport();

	void Construct(const FArguments& Arg);
	void SetViewportSkeletal(USkeletalMesh* InSkeletaMesh, TArray<FSkeletalMaterial> Variants);
private:
	TSharedPtr<FEditorViewportClient> ViewportClient;
	TSharedPtr<FAssetEditorModeManager> AssetEditorModeManager;
	TSharedPtr<FAdvancedPreviewScene> AdvancedPreviewScene;

	TSharedRef<class SEditorViewport> GetViewportWidget() override;
	TSharedPtr<FExtender> GetExtenders() const override;
	void OnFloatingButtonClicked() override;

	AActor* PreviewActor;

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
};
