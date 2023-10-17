// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleVariantEditorViewport.h"
#include "AssetEditorModeManager.h"
#include "AdvancedPreviewScene.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "AnimationRecorder.h"
#include "Animation/SkeletalMeshActor.h"


DoodleVariantEditorPreviewScene::DoodleVariantEditorPreviewScene()
    : FAdvancedPreviewScene(

        FAdvancedPreviewScene::ConstructionValues{}.SetCreatePhysicsScene(false).ShouldSimulatePhysics(false)

    ) {
    SetFloorVisibility(true);
    GetWorld()->GetWorldSettings()->NotifyBeginPlay();
    GetWorld()->GetWorldSettings()->NotifyMatchStarted();
    GetWorld()->GetWorldSettings()->SetActorHiddenInGame(false);
    GetWorld()->bBegunPlay = true;
}

void DoodleVariantEditorPreviewScene::Tick(float InDeltaTime) {
    FAdvancedPreviewScene::Tick(InDeltaTime);

    if (!GIntraFrameDebuggingGameThread) {
        GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
    }
}
//---------------------------
DoodleVariantEditorViewportClient::DoodleVariantEditorViewportClient(FAssetEditorModeManager* InAssetEditorModeManager, FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget)
    : FEditorViewportClient{ InAssetEditorModeManager, InPreviewScene, InEditorViewportWidget } {
    SetViewMode(VMI_Lit);
    SetViewportType(LVT_Perspective);

    SetInitialViewTransform(LVT_Perspective, EditorViewportDefs::DefaultPerspectiveViewLocation, EditorViewportDefs::DefaultPerspectiveViewRotation, EditorViewportDefs::DefaultPerspectiveFOVAngle);

    SetRealtime(true);
    bSetListenerPosition = false;
    SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
    SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);
    EngineShowFlags.SetCompositeEditorPrimitives(true);
}

void DoodleVariantEditorViewportClient::Draw(FViewport* InViewport, FCanvas* Canvas) {
    FEditorViewportClient::Draw(InViewport, Canvas);
}

void DoodleVariantEditorViewportClient::Tick(float DeltaSeconds) {
    // GetPreviewScene()->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
    FEditorViewportClient::Tick(DeltaSeconds);
    GetPreviewScene()->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
    // if (DeltaSeconds > 0) {
    //}
}
//-----------------------------------------
void DoodleVariantEditorViewportToolBar::Construct(
    const FArguments& InArgs, TSharedPtr<DoodleVariantEditorViewport> InRealViewport
) {
    SCommonEditorViewportToolbarBase::Construct(SCommonEditorViewportToolbarBase::FArguments(), InRealViewport);
}
//--------------------------
DoodleVariantEditorViewport::DoodleVariantEditorViewport()
{
}

DoodleVariantEditorViewport::~DoodleVariantEditorViewport()
{
    if (PreviewActor)
        PreviewActor->Destroy();
}

TSharedRef<class SEditorViewport> DoodleVariantEditorViewport::GetViewportWidget() { return SharedThis(this); }

TSharedPtr<FExtender> DoodleVariantEditorViewport::GetExtenders() const {
    TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
    return Result;
}

void DoodleVariantEditorViewport::OnFloatingButtonClicked() {}

void DoodleVariantEditorViewport::Construct(const FArguments& Arg) {
    //DoodleCreateCharacterConfigAttr = Arg._DoodleCreateCharacterConfigAttr;
    AssetEditorModeManager = MakeShared<FAssetEditorModeManager>();
    AssetEditorModeManager->SetWidgetModeOverride(UE::Widget::EWidgetMode::WM_Translate);
    SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<FEditorViewportClient> DoodleVariantEditorViewport::MakeEditorViewportClient() {
    if (!AdvancedPreviewScene) AdvancedPreviewScene = MakeShared<DoodleVariantEditorPreviewScene>();

    if (!ViewportClient)
        ViewportClient =MakeShared<DoodleVariantEditorViewportClient>(AssetEditorModeManager.Get(), AdvancedPreviewScene.Get(), SharedThis(this));
    ViewportClient->SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
    ViewportClient->SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);
    ViewportClient->EngineShowFlags.SetCompositeEditorPrimitives(true);
    //--------------------------------------------
    PreviewActor = ViewportClient->GetWorld()->SpawnActor<AActor>();
    UActorComponent* L_Component = PreviewActor->AddComponentByClass(UDebugSkelMeshComponent::StaticClass(), true, FTransform{}, true);
    L_Component->RegisterComponent();
    //-----------------------
    ViewportClient->ViewportType = LVT_Perspective;
    ViewportClient->bSetListenerPosition = false;
    return ViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> DoodleVariantEditorViewport::MakeViewportToolbar() 
{
    return SNew(DoodleVariantEditorViewportToolBar, SharedThis(this)).Cursor(EMouseCursor::Default);
}

void DoodleVariantEditorViewport::SetViewportSkeletal(USkeletalMesh* InSkeletaMesh, TArray<FSkeletalMaterial> Variants) {
    if(PreviewActor)
    {
        UDebugSkelMeshComponent* Component = PreviewActor->GetComponentByClass<UDebugSkelMeshComponent>();
        Component->SetSkeletalMesh(InSkeletaMesh);
        TArray<FSkeletalMaterial> L_List = Variants;
        for (int i = 0;i < L_List.Num();i++)
        {
            Component->SetMaterial(i, L_List[i].MaterialInterface);
        }
        Component->PostApplyToComponent();
    }
}