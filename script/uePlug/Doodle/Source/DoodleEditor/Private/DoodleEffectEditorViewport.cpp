// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleEffectEditorViewport.h"
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
#include "GameFramework/WorldSettings.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/TextureCube.h"

DoodleEffectEditorPreviewScene::DoodleEffectEditorPreviewScene()
 : FAdvancedPreviewScene(FAdvancedPreviewScene::ConstructionValues{}.SetCreatePhysicsScene(false).ShouldSimulatePhysics(false)) 
{
    SetFloorVisibility(true);
    GetWorld()->GetWorldSettings()->NotifyBeginPlay();
    GetWorld()->GetWorldSettings()->NotifyMatchStarted();
    GetWorld()->GetWorldSettings()->SetActorHiddenInGame(false);
    GetWorld()->bBegunPlay = true;
}

void DoodleEffectEditorPreviewScene::Tick(float InDeltaTime) 
{
    FAdvancedPreviewScene::Tick(InDeltaTime);
    if (!GIntraFrameDebuggingGameThread) 
    {
        GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
    }
}

//---------------------------
DoodleEffectEditorViewportClient::DoodleEffectEditorViewportClient(FAssetEditorModeManager* InAssetEditorModeManager, FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget)
    : FEditorViewportClient{ InAssetEditorModeManager, InPreviewScene, InEditorViewportWidget } 
{
    SetViewMode(VMI_Lit);
    SetViewportType(LVT_Perspective);

    SetInitialViewTransform(LVT_Perspective, EditorViewportDefs::DefaultPerspectiveViewLocation, EditorViewportDefs::DefaultPerspectiveViewRotation, EditorViewportDefs::DefaultPerspectiveFOVAngle);

    SetRealtime(true);
    bSetListenerPosition = false;
    SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
    SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);
    EngineShowFlags.SetCompositeEditorPrimitives(true);
}

void DoodleEffectEditorViewportClient::Draw(FViewport* InViewport, FCanvas* Canvas) 
{
    FEditorViewportClient::Draw(InViewport, Canvas);
}

void DoodleEffectEditorViewportClient::Tick(float DeltaSeconds) {
    FEditorViewportClient::Tick(DeltaSeconds);
    GetPreviewScene()->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
    //----------------------------------
}

void DoodleEffectEditorViewportToolBar::Construct(const FArguments& InArgs, TSharedPtr<DoodleEffectEditorViewport> InRealViewport) 
{
    SCommonEditorViewportToolbarBase::Construct(SCommonEditorViewportToolbarBase::FArguments(), InRealViewport);
}
//--------------------------
DoodleEffectEditorViewport::~DoodleEffectEditorViewport()
{
    if (PreviewActor)
        PreviewActor->RemoveFromRoot();
}

TSharedRef<class SEditorViewport> DoodleEffectEditorViewport::GetViewportWidget() { return SharedThis(this); }

TSharedPtr<FExtender> DoodleEffectEditorViewport::GetExtenders() const {
    TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
    return Result;
}

void DoodleEffectEditorViewport::OnFloatingButtonClicked() {}

void DoodleEffectEditorViewport::Construct(const FArguments& Arg) {
    //DoodleCreateCharacterConfigAttr = Arg._DoodleCreateCharacterConfigAttr;
    AssetEditorModeManager = MakeShared<FAssetEditorModeManager>();
    AssetEditorModeManager->SetWidgetModeOverride(UE::Widget::EWidgetMode::WM_Translate);
    //----------------------
    SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<FEditorViewportClient> DoodleEffectEditorViewport::MakeEditorViewportClient() 
{
    if (!PreviewScene)PreviewScene = MakeShared<DoodleEffectEditorPreviewScene>();
    //----------------
    if (!ViewportClient)
        ViewportClient =MakeShared<DoodleEffectEditorViewportClient>(AssetEditorModeManager.Get(), PreviewScene.Get(), SharedThis(this));
    ViewportClient->SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
    ViewportClient->SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);
    ViewportClient->EngineShowFlags.SetCompositeEditorPrimitives(true);
    //-------------
    PreviewScene->SetFloorVisibility(false);
    PreviewScene->SetEnvironmentVisibility(false,false);
    UTextureCube* CubemapMap = LoadObject<UTextureCube>(nullptr, TEXT("/Engine/EngineResources/GrayTextureCube.GrayTextureCube"));
    PreviewScene->SetSkyCubemap(CubemapMap);
    PreviewScene->SetLightBrightness(1);
    //-----------------------
    ViewportClient->ViewportType = LVT_Perspective;
    ViewportClient->bSetListenerPosition = false;

    return ViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> DoodleEffectEditorViewport::MakeViewportToolbar() 
{
    return SNew(DoodleEffectEditorViewportToolBar, SharedThis(this)).Cursor(EMouseCursor::Default);
}

void DoodleEffectEditorViewport::SetViewportData(TObjectPtr<UObject> ParticleObj)
{
    if (ParticleObj->GetClass() == UNiagaraSystem::StaticClass())
    {
        PreviewActor = ViewportClient->GetWorld()->SpawnActor<AActor>();
        PreviewActor->AddToRoot();
        UActorComponent* L_Component = PreviewActor->AddComponentByClass(UNiagaraComponent::StaticClass(), true, FTransform{}, true);
        L_Component->RegisterComponent();
        UNiagaraComponent* Component = CastChecked<UNiagaraComponent>(L_Component);
        PreviewActor->SetRootComponent(Component);
        PreviewScene->AddComponent(Component, FTransform::Identity);
        Component->SetAsset(Cast<UNiagaraSystem>(ParticleObj.Get()));
    }
    if (ParticleObj->GetClass() == UParticleSystem::StaticClass())
    {
        PreviewActor = ViewportClient->GetWorld()->SpawnActor<AActor>();
        PreviewActor->AddToRoot();
        UActorComponent* P_Component = PreviewActor->AddComponentByClass(UParticleSystemComponent::StaticClass(), true, FTransform{}, true);
        P_Component->RegisterComponent();
        UParticleSystemComponent* PComponent = CastChecked<UParticleSystemComponent>(P_Component);
        PreviewActor->SetRootComponent(PComponent);
        PreviewScene->AddComponent(PComponent, FTransform::Identity);
        PComponent->SetTemplate(Cast<UParticleSystem>(ParticleObj.Get()));
        PComponent->InitializeSystem();
        PComponent->ActivateSystem();
    }
    if (ParticleObj->GetClass() == UBlueprint::StaticClass())
    {
        UBlueprint* PreviewBlueprint = Cast<UBlueprint>(ParticleObj.Get());
        if (PreviewBlueprint && PreviewBlueprint->GeneratedClass && PreviewBlueprint->GeneratedClass->IsChildOf(AActor::StaticClass()))
        {
            FVector SpawnLocation = FVector::ZeroVector;
            FRotator SpawnRotation = FRotator::ZeroRotator;
            FActorSpawnParameters SpawnInfo;
            SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            SpawnInfo.bNoFail = true;
            SpawnInfo.ObjectFlags = RF_Transient | RF_Transactional;
            ///------------------------------
            FMakeClassSpawnableOnScope TemporarilySpawnable(PreviewBlueprint->GeneratedClass);
            PreviewActor = PreviewScene->GetWorld()->SpawnActor(PreviewBlueprint->GeneratedClass, &SpawnLocation, &SpawnRotation, SpawnInfo);
            PreviewActor->AddToRoot();
            //---------------------
            if (PreviewBlueprint->SimpleConstructionScript != nullptr)
            {
                PreviewBlueprint->SimpleConstructionScript->SetComponentEditorActorInstance(PreviewActor.Get());
            }
        }
    }
}

void DoodleEffectEditorViewport::OnResetViewport()
{
    if (PreviewActor)
    {
        for (UActorComponent* Component : PreviewActor->GetComponents())
        {
            if (Component && Component->IsA(UNiagaraComponent::StaticClass()))
            {
                //Component->Activate(true);
                UNiagaraComponent* NiagaraSystemComponent = Cast<UNiagaraComponent>(Component);
                NiagaraSystemComponent->Activate(true);
                NiagaraSystemComponent->ReregisterComponent();
            }
            if (Component && Component->IsA(UParticleSystemComponent::StaticClass()))
            {
                //Component->Activate(true);
                UParticleSystemComponent* ParticleSystemComponent = Cast<UParticleSystemComponent>(Component);
                //---------
                ParticleSystemComponent->ResetParticles();
                ParticleSystemComponent->SetManagingSignificance(true);
                ParticleSystemComponent->ActivateSystem();
                if (ParticleSystemComponent->Template)
                {
                    ParticleSystemComponent->Template->bShouldResetPeakCounts = true;
                }
                ParticleSystemComponent->bIsViewRelevanceDirty = true;
                ParticleSystemComponent->CachedViewRelevanceFlags.Empty();
                ParticleSystemComponent->ConditionalCacheViewRelevanceFlags();
            }
        }
    }
}