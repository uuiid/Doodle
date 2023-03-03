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
class SCreateCharacterMianUI;
class FAssetEditorModeManager;
class UDoodleCreateCharacterConfig;

class FCharacterEditorPreviewScene : public FAdvancedPreviewScene {
 public:
  FCharacterEditorPreviewScene();
  // FAdvancedPreviewScene 接口
  virtual void Tick(float InDeltaTime) override;
};

class FCharacterEditorViewportClient : public FEditorViewportClient {
 public:
  FCharacterEditorViewportClient(FAssetEditorModeManager* InAssetEditorModeManager, FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget);

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
  SLATE_BEGIN_ARGS(SCharacterEditorViewport)
      : _CreateCharacterMianUI(),
        _DoodleCreateCharacterConfigAttr() {}

  SLATE_ATTRIBUTE(SCreateCharacterMianUI*, CreateCharacterMianUI)

  SLATE_ATTRIBUTE(UDoodleCreateCharacterConfig*, DoodleCreateCharacterConfigAttr)
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
  void MoveBoneTransform(const FName& In_Bone, float In_Value);

 protected:
  virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
  virtual TSharedPtr<SWidget> MakeViewportToolbar() override;

 private:
  TSharedPtr<FEditorViewportClient> LevelViewportClient;
  TSharedPtr<FAdvancedPreviewScene> AdvancedPreviewScene;
  AActor* PreviewActor;

  UDebugSkelMeshComponent* ShowSkeletaMesh;
  USkeletalMesh* SkeletalMesh;
  // 模式
  TSharedPtr<FAssetEditorModeManager> AssetEditorModeManager;
  // 显示的动画
  TObjectPtr<UAnimSequence> ShowAnimSequence;

  // 主要窗口指针
  TAttribute<SCreateCharacterMianUI*> CreateCharacterMianUI;
  // 主要的配置文件
  TAttribute<UDoodleCreateCharacterConfig*> DoodleCreateCharacterConfigAttr;

  static FName G_Name;
};