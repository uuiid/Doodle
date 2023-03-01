#pragma once

#include "AdvancedPreviewScene.h"
#include "CoreMinimal.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "SEditorViewport.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
class FCharacterEditorViewportClient;
class SCharacterEditorViewportToolBar;
class SCharacterEditorViewport;
class ASkeletalMeshActor;

class FCharacterEditorPreviewScene : public FAdvancedPreviewScene {
 public:
  FCharacterEditorPreviewScene();
  // FAdvancedPreviewScene 接口
  virtual void Tick(float InDeltaTime) override;
};

class FCharacterEditorViewportClient : public FEditorViewportClient {
 public:
  using FEditorViewportClient::FEditorViewportClient;
  virtual void Draw(FViewport* InViewport, FCanvas* Canvas) override;

  virtual void Tick(float DeltaSeconds) override;
};

class SCharacterEditorViewportToolBar : public SCommonEditorViewportToolbarBase {
 public:
  SLATE_BEGIN_ARGS(SCharacterEditorViewportToolBar) {}

  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs, TSharedPtr<SCharacterEditorViewport> InRealViewport);
};

class SCharacterEditorViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider {
 public:
  SLATE_BEGIN_ARGS(SCharacterEditorViewport) {}
  SLATE_END_ARGS()

  //
  void Construct(const FArguments& Arg);

  // 开始工具栏接口
  TSharedRef<class SEditorViewport> GetViewportWidget() override;
  TSharedPtr<FExtender> GetExtenders() const override;
  void OnFloatingButtonClicked() override;
  // 结束

  // 临时接口

  void doodle_test(const FName& In_Bone, float in_value);

  void SetViewportSkeletal(USkeletalMesh* InSkeletaMesh);

 protected:
  virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
  virtual TSharedPtr<SWidget> MakeViewportToolbar() override;

 private:
  TSharedPtr<FEditorViewportClient> LevelViewportClient;
  TSharedPtr<FAdvancedPreviewScene> AdvancedPreviewScene;
  AActor* PreviewActor;

  UDebugSkelMeshComponent* ShowSkeletaMesh;
  USkeletalMesh* SkeletalMesh;
  static FName G_Name;
};