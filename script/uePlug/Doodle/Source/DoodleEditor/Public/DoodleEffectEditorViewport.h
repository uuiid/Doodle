// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "AdvancedPreviewScene.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "SkeletalRenderPublic.h"


class DoodleEffectEditorViewport;

class DoodleEffectEditorPreviewScene : public FAdvancedPreviewScene {
public:
	DoodleEffectEditorPreviewScene();
	// FAdvancedPreviewScene 
	virtual void Tick(float InDeltaTime) override;
};

class DoodleEffectEditorViewportClient : public FEditorViewportClient {
public:
	DoodleEffectEditorViewportClient(FAssetEditorModeManager* InAssetEditorModeManager, FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget);

	virtual void Draw(FViewport* InViewport, FCanvas* Canvas) override;

	virtual void Tick(float DeltaSeconds) override;
};

class DoodleEffectEditorViewportToolBar : public SCommonEditorViewportToolbarBase {
public:
	SLATE_BEGIN_ARGS(DoodleEffectEditorViewportToolBar) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<DoodleEffectEditorViewport> InRealViewport);
};

/**
 * 
 */
class DOODLEEDITOR_API DoodleEffectEditorViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{
public:
	~DoodleEffectEditorViewport();

	void Construct(const FArguments& Arg);
	void SetViewportData(TObjectPtr<UObject> ParticleObj);
	void OnResetViewport();

	FVector2D ComputeDesiredSize(float) const override { return FVector2D(768, 768); }
	TObjectPtr<AActor> PreviewActor;
private:
	TSharedPtr<DoodleEffectEditorViewportClient> ViewportClient;
	TSharedPtr<FAssetEditorModeManager> AssetEditorModeManager;
	TSharedPtr<DoodleEffectEditorPreviewScene> PreviewScene;

	TSharedRef<class SEditorViewport> GetViewportWidget() override;
	TSharedPtr<FExtender> GetExtenders() const override;
	void OnFloatingButtonClicked() override;
protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
};
