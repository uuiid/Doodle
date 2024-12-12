// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleVariantEditorViewport.h"
#include "AssetEditorModeManager.h"
#include "AdvancedPreviewScene.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "AnimationRecorder.h"
#include "Animation/SkeletalMeshActor.h"

#include "DoodleCommands.h"
#include "CameraController.h"
#include "SkeletalRenderPublic.h"
#include "EditorViewportCommands.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Engine/SkinnedAssetCommon.h"

DoodleVariantEditorPreviewScene::DoodleVariantEditorPreviewScene()
    : FAdvancedPreviewScene(FAdvancedPreviewScene::ConstructionValues{}.SetCreatePhysicsScene(false).ShouldSimulatePhysics(false)) {
    SetFloorVisibility(true);
    GetWorld()->GetWorldSettings()->NotifyBeginPlay();
    GetWorld()->GetWorldSettings()->NotifyMatchStarted();
    GetWorld()->GetWorldSettings()->SetActorHiddenInGame(false);
    GetWorld()->SetBegunPlay(true);
}

void DoodleVariantEditorPreviewScene::Tick(float InDeltaTime) {
    FAdvancedPreviewScene::Tick(InDeltaTime);

    if (!GIntraFrameDebuggingGameThread) {
        GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
    }
}

void DoodleVariantEditorPreviewScene::SetPreviewMeshComponent(UDebugSkelMeshComponent* InSkeletalMeshComponent)
{
    SkeletalMeshComponent = InSkeletalMeshComponent;

    if (SkeletalMeshComponent)
    {
        SkeletalMeshComponent->SelectionOverrideDelegate = UPrimitiveComponent::FSelectionOverride::CreateRaw(this, &DoodleVariantEditorPreviewScene::PreviewComponentSelectionOverride);
        SkeletalMeshComponent->PushSelectionToProxy();
    }
}

bool DoodleVariantEditorPreviewScene::PreviewComponentSelectionOverride(const UPrimitiveComponent* InComponent) const
{
    if (InComponent == SkeletalMeshComponent)
    {
        const USkeletalMeshComponent* Component = CastChecked<USkeletalMeshComponent>(InComponent);
        return (Component->GetSelectedEditorSection() != INDEX_NONE || Component->GetSelectedEditorMaterial() != INDEX_NONE);
    }

    return false;
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
    FEditorViewportClient::Tick(DeltaSeconds);
    GetPreviewScene()->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
    //----------------------------------
}

FBox DoodleVariantEditorViewportClient::ComputeBoundingBoxForSelectedEditorSection(UDebugSkelMeshComponent* PreviewMeshComponent)
{
    if (!PreviewMeshComponent)
    {
        return FBox(ForceInitToZero);
    }
    //-------------------------
    USkeletalMesh* const SkeletalMesh = PreviewMeshComponent->GetSkeletalMeshAsset();
    FSkeletalMeshObject* const MeshObject = PreviewMeshComponent->MeshObject;
    if (!SkeletalMesh || !MeshObject)
    {
        return FBox(ForceInitToZero);
    }
    //------------------------------
    const int32 LODLevel = PreviewMeshComponent->GetPredictedLODLevel();
    const int32 SelectedEditorSection = PreviewMeshComponent->GetSelectedEditorSection();
    const FSkeletalMeshRenderData& SkelMeshRenderData = MeshObject->GetSkeletalMeshRenderData();
    //--------------------------
    const FSkeletalMeshLODRenderData& LODData = SkelMeshRenderData.LODRenderData[LODLevel];
    const FSkelMeshRenderSection& SelectedSectionSkelMesh = LODData.RenderSections[SelectedEditorSection];
    // Get us vertices from the entire LOD model.
    TArray<FFinalSkinVertex> SkinnedVertices;
    PreviewMeshComponent->GetCPUSkinnedVertices(SkinnedVertices, LODLevel);
    //------------------------------TransformVertexPositionsToWorld
    TransformVertexPositionsToWorld(PreviewMeshComponent,SkinnedVertices);
    //-------------------------------
    // Find out which of these the selected section actually uses.
    TArray<int32> VertexIndices;
    GetAllVertexIndicesUsedInSection(*LODData.MultiSizeIndexContainer.GetIndexBuffer(), SelectedSectionSkelMesh, VertexIndices);
    // Get their bounds.
    FBox BoundingBox(ForceInitToZero);
    for (int32 Index = 0; Index < VertexIndices.Num(); ++Index)
    {
        const int32 VertexIndex = VertexIndices[Index];
        BoundingBox += (FVector)SkinnedVertices[VertexIndex].Position;
    }
    return BoundingBox;
}

void DoodleVariantEditorViewportClient::TransformVertexPositionsToWorld(UDebugSkelMeshComponent* PreviewMeshComponent,TArray<FFinalSkinVertex>& LocalVertices)
{
    const FTransform& LocalToWorldTransform = PreviewMeshComponent->GetComponentTransform();
    //--------------------------------------
    for (int32 VertexIndex = 0; VertexIndex < LocalVertices.Num(); ++VertexIndex)
    {
        FVector3f& VertexPosition = LocalVertices[VertexIndex].Position;
        VertexPosition = (FVector3f)LocalToWorldTransform.TransformPosition((FVector)VertexPosition);
    }
}

void DoodleVariantEditorViewportClient::GetAllVertexIndicesUsedInSection(const FRawStaticIndexBuffer16or32Interface& IndexBuffer,
    const FSkelMeshRenderSection& SkelMeshSection,
    TArray<int32>& OutIndices)
{
    const uint32 BaseIndex = SkelMeshSection.BaseIndex;
    const int32 NumWedges = SkelMeshSection.NumTriangles * 3;

    for (int32 WedgeIndex = 0; WedgeIndex < NumWedges; ++WedgeIndex)
    {
        const int32 VertexIndexForWedge = IndexBuffer.Get(SkelMeshSection.BaseIndex + WedgeIndex);
        OutIndices.Add(VertexIndexForWedge);
    }
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
    //----------------------
    SEditorViewport::Construct(SEditorViewport::FArguments());
}

void DoodleVariantEditorViewport::OnFocusViewportToSelection(){
    if (!PreviewScene) return;

    UDebugSkelMeshComponent* PreviewMeshComponent = PreviewScene->SkeletalMeshComponent;
    //--------------------
    FIntPoint ViewportSize(FIntPoint::ZeroValue);
    if (ViewportClient->Viewport != nullptr)
    {
        ViewportSize = ViewportClient->Viewport->GetSizeXY();
    }
    //---------------
    if (USkeletalMesh* const SkelMesh = PreviewMeshComponent->GetSkeletalMeshAsset())
    {
        if (PreviewMeshComponent->GetSelectedEditorSection() != INDEX_NONE)
        {
            const FBox SelectedSectionBounds = ViewportClient->ComputeBoundingBoxForSelectedEditorSection(PreviewMeshComponent);

            if (SelectedSectionBounds.IsValid)
            {
                ViewportClient->FocusViewportOnBox(SelectedSectionBounds);
            }
            return;
        }
    }
    FBoxSphereBounds Bounds = PreviewMeshComponent->CalcGameBounds(FTransform::Identity);
    FSphere Sphere = Bounds.GetSphere();
    //-------------------------
    FBox Box(Sphere.Center - FVector(Sphere.W, 0.0f, 0.0f), Sphere.Center + FVector(Sphere.W, 0.0f, 0.0f));
    //-------------------
    ViewportClient->FocusViewportOnBox(Box, true);
}

TSharedRef<FEditorViewportClient> DoodleVariantEditorViewport::MakeEditorViewportClient() 
{
    if (!PreviewScene)PreviewScene = MakeShared<DoodleVariantEditorPreviewScene>();
    //----------------
    if (!ViewportClient)
        ViewportClient =MakeShared<DoodleVariantEditorViewportClient>(AssetEditorModeManager.Get(), PreviewScene.Get(), SharedThis(this));
    ViewportClient->SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
    ViewportClient->SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);
    ViewportClient->EngineShowFlags.SetCompositeEditorPrimitives(true);
    //--------------------------------------------
    PreviewActor = ViewportClient->GetWorld()->SpawnActor<AActor>();
    UActorComponent* L_Component = PreviewActor->AddComponentByClass(UDebugSkelMeshComponent::StaticClass(), true, FTransform{}, true);
    L_Component->RegisterComponent();

    UDebugSkelMeshComponent* SkeletalMeshComponent = CastChecked<UDebugSkelMeshComponent>(L_Component);
    PreviewActor->SetRootComponent(SkeletalMeshComponent);
    PreviewScene->AddComponent(SkeletalMeshComponent, FTransform::Identity);
    PreviewScene->SetPreviewMeshComponent(SkeletalMeshComponent);

    //-----------------------
    ViewportClient->ViewportType = LVT_Perspective;
    ViewportClient->bSetListenerPosition = false;

    return ViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> DoodleVariantEditorViewport::MakeViewportToolbar() 
{
    return SNew(DoodleVariantEditorViewportToolBar, SharedThis(this)).Cursor(EMouseCursor::Default);
}

void DoodleVariantEditorViewport::SetViewportSkeletal(USkeletalMesh* InSkeletaMesh, TArray<FSkeletalMaterial> Variants) 
{
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

//------------------------
ECheckBoxState DoodleVariantEditorViewport::IsIsolateMaterialEnabled(int MaterialIndex)
{
    ECheckBoxState State = ECheckBoxState::Unchecked;
    const UDebugSkelMeshComponent* MeshComponent = PreviewScene->SkeletalMeshComponent;
    if (MeshComponent)
    {
        State = MeshComponent->GetMaterialPreview() == MaterialIndex ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return State;
}

void DoodleVariantEditorViewport::OnMaterialIsolatedChanged(ECheckBoxState NewState, int MaterialIndex)
{
    UDebugSkelMeshComponent* MeshComponent = PreviewScene->SkeletalMeshComponent;
    if (MeshComponent)
    {
        if (NewState == ECheckBoxState::Checked)
        {
            MeshComponent->SetMaterialPreview(MaterialIndex);
            if (MeshComponent->GetSelectedEditorMaterial() != MaterialIndex)
            {
                MeshComponent->SetSelectedEditorMaterial(INDEX_NONE);
            }
            MeshComponent->SetSectionPreview(INDEX_NONE);
            MeshComponent->SetSelectedEditorSection(MaterialIndex);
        }
        else if (NewState == ECheckBoxState::Unchecked)
        {
            MeshComponent->SetMaterialPreview(INDEX_NONE);
            MeshComponent->SetSelectedEditorSection(INDEX_NONE);
        }
    }
}
