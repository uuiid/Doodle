#pragma once

#include "AdvancedPreviewScene.h"
#include "CoreMinimal.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "SEditorViewport.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FCharacterEditorViewportClient;
class SCharacterEditorViewportToolBar;
class SCharacterEditorViewport;

class FCharacterEditorPreviewScene : public FAdvancedPreviewScene {
 public:
  FCharacterEditorPreviewScene();
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

  // ���������ݴ�������
  void Construct(const FArguments& Arg, USkeletalMesh* InSkeletaMesh);

  // ��ʼ�������ӿ�
  TSharedRef<class SEditorViewport> GetViewportWidget() override;
  TSharedPtr<FExtender> GetExtenders() const override;
  void OnFloatingButtonClicked() override;
  // �����������ӿ�

 protected:
  virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
  virtual TSharedPtr<SWidget> MakeViewportToolbar() override;

 private:
  TSharedPtr<FEditorViewportClient> LevelViewportClient;
  TSharedPtr<FAdvancedPreviewScene> AdvancedPreviewScene;
  ASkeletalMeshActor* PreviewActor;

  USkeletalMesh* ShowSkeletaMesh;
};