// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleConfigLightActor.h"

#include "Animation/SkeletalMeshActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "DoodleConfigLight.h"
#include "Engine/Light.h"
#include "Engine/World.h"

#if WITH_EDITOR
#if ENGINE_MINOR_VERSION >= 26
#include "AssetRegistry/AssetRegistryModule.h"
#endif
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#endif

// 添加编辑器显示图标
#include "Components/ArrowComponent.h"

#define LOCTEXT_NAMESPACE "doodle"

ADoodleConfigLightActor::ADoodleConfigLightActor() : p_light_list() {
  RootComponent = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
  RootComponent->SetMobility(EComponentMobility::Movable);
  // RootComponent->SetupAttachment(RootComponent);

  for (int i = 0; i < 3; i++) {
    auto com_1         = CreateDefaultSubobject<UArrowComponent>("com_1" + i);

    auto l_en          = com_1->ArrowLength * 2;
    com_1->ArrowLength = l_en;
    com_1->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
    com_1->SetArrowColor({1, 0, 0});
    com_1->SetWorldRotation({0, 0, 0});
    com_1->SetWorldLocation({-(l_en / 2), (float)((i - 1) * 20), 0});
    // com_1->SetEditorScale(2);
  }
}

#if WITH_EDITOR

bool ADoodleConfigLightActor::OpenSaveDialog(const FString& InDefaultPath, const FString& InNewNameSuggestion, FString& OutPackageName) {
  FSaveAssetDialogConfig SaveAssetDialogConfig;
  {
    SaveAssetDialogConfig.DefaultPath      = InDefaultPath;
    SaveAssetDialogConfig.DefaultAssetName = InNewNameSuggestion;
    SaveAssetDialogConfig.AssetClassNames.Add(
        UDoodleConfigLight::StaticClass()->GetFName()
    );
    SaveAssetDialogConfig.ExistingAssetPolicy =
        ESaveAssetDialogExistingAssetPolicy::AllowButWarn;
    SaveAssetDialogConfig.DialogTitleOverride =
        LOCTEXT("SaveConfigPresetDialogTitle", "Save Doodle light Config");
  }

  FContentBrowserModule& ContentBrowserModule =
      FModuleManager::LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser"
      );
  FString SaveObjectPath =
      ContentBrowserModule.Get().CreateModalSaveAssetDialog(
          SaveAssetDialogConfig
      );

  if (!SaveObjectPath.IsEmpty()) {
    OutPackageName = FPackageName::ObjectPathToPackageName(SaveObjectPath);
    return true;
  }

  return false;
}

UObject* ADoodleConfigLightActor::OpenDialog(
    const FString& InDefaultPath, const FString& InNewNameSuggestion
) {
  FOpenAssetDialogConfig OpenAssetDialogConfig;
  {
    OpenAssetDialogConfig.DefaultPath = InDefaultPath;
    OpenAssetDialogConfig.AssetClassNames.Add(
        UDoodleConfigLight::StaticClass()->GetFName()
    );
    OpenAssetDialogConfig.bAllowMultipleSelection = false;
    OpenAssetDialogConfig.DialogTitleOverride =
        LOCTEXT("OpenConfigPresetDialogTitle", "Open Doodle light Config");
  }

  FContentBrowserModule& ContentBrowserModule =
      FModuleManager::LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser"
      );
  TArray<FAssetData> l_AssetData_ =
      ContentBrowserModule.Get().CreateModalOpenAssetDialog(
          OpenAssetDialogConfig
      );

  if (l_AssetData_.IsValidIndex(0)) {
    // OutObjName = FPackageName::ObjectPathToPackageName(l_AssetData_[0]);
    return l_AssetData_[0].GetAsset();
  }

  return {};
}

#endif  // WITH_EDITOR
void ADoodleConfigLightActor::SaveConfig() {
#if WITH_EDITOR
  FString DialogStartPath{TEXT("/Game")};
  FString DefaultName{"doodle"};
  FString UniquePackageName;
  FString UniqueAssetName;

  FAssetToolsModule& AssetToolsModule =
      FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
  AssetToolsModule.Get().CreateUniqueAssetName(DialogStartPath / DefaultName, TEXT(""), UniquePackageName, UniqueAssetName);

  FString DialogStartName = FPaths::GetCleanFilename(UniqueAssetName);

  FString UserPackageName;
  FString NewPackageName;

  // get destination for asset
  bool bFilenameValid = false;
  while (!bFilenameValid) {
    if (!OpenSaveDialog(DialogStartPath, DialogStartName, UserPackageName)) {
      return;
    }

    NewPackageName =
        UserPackageName;  // FString::Format(*UserPackageName, FormatArgs);

    FText OutError;
    bFilenameValid =
        FFileHelper::IsFilenameValidForSaving(NewPackageName, OutError);
  }
  const FString NewAssetName =
      FPackageName::GetLongPackageAssetName(NewPackageName);
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26
  UPackage* NewPackage = CreatePackage(*NewPackageName);

#else if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 26
  UPackage* NewPackage = CreatePackage(nullptr, *NewPackageName);
#endif

  UDoodleConfigLight* NewPreset = NewObject<UDoodleConfigLight>(
      NewPackage, UDoodleConfigLight::StaticClass(), *NewAssetName,
      RF_Public | RF_Standalone | RF_Transactional
  );
  if (NewPreset) {
    // for (auto it = p_light_list.CreateIterator(); it; ++it) {
    //  ///// <summary>
    //  ///// 这里要复制出去obj,
    //  ///// 不可以使用关卡中的指针,要不然重新创建关卡时会内存泄露
    //  ///// </summary>
    //  // ULightComponent* k_l = DuplicateObject(it->light, NewPackage);
    //  ////
    //  ///
    //  k_l->DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);

    //  //// UE_LOG(LogTemp, Log, TEXT("save tran: %s"),
    //  ////       *k_l->GetTransform().ToString());

    //  // auto l_f = it->weight;
    //  // NewPreset->p_light.Add({k_l, l_f});
    //}
    NewPreset->p_Actor = DuplicateObject(this, NewPackage);
    NewPreset->p_Actor->DetachFromActor(
        FDetachmentTransformRules::KeepRelativeTransform
    );

    NewPreset->MarkPackageDirty();
#if ENGINE_MINOR_VERSION >= 26
    FAssetRegistryModule::AssetCreated(NewPreset);
#endif
  }
#endif  // WITH_EDITOR
}

void ADoodleConfigLightActor::LoadConfig() {
#if WITH_EDITOR
  FString DialogStartPath{TEXT("/Game")};
  FString DefaultName{"doodle"};
  auto* l_config =
      Cast<UDoodleConfigLight>(OpenDialog(DefaultName / DefaultName, TEXT("")));
  if (l_config) {
    UE_LOG(LogTemp, Log, TEXT("load file name: %s"), *l_config->GetPathName());
    FActorSpawnParameters l_par{};
    l_par.Template = l_config->p_Actor;
    GWorld->SpawnActor<ADoodleConfigLightActor>(l_par);
    this->NotifyActorOnClicked();

    // for (auto it = p_light_list.CreateIterator();
    // it; ++it) {
    //  it->light->UnregisterComponent();
    //  it->light->DestroyComponent();
    //}
    // p_light_list.Empty();
    // for (auto it = l_config->p_light.CreateIterator(); it; ++it) {
    //  auto l_com = DuplicateObject(it->light, this);
    //  l_com->RegisterComponent();
    //  l_com->AttachToComponent(
    //      RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
    //  p_light_list.Add({l_com, it->weight});
    //  // UE_LOG(LogTemp, Log, TEXT("load org tran: %s"),
    //  //       *(it->light->GetTransform().ToString()));
    //  // UE_LOG(LogTemp, Log, TEXT("load tran: %s"),
    //  //       *(k_a->GetTransform().ToString()));
    //  // LoadObject<ALight>();
    //}
  }

#endif  // WITH_EDITOR
}

#if WITH_EDITOR
void ADoodleConfigLightActor::PostEditChangeProperty(
    struct FPropertyChangedEvent& PropertyChangeEvent
) {
  Super::PostEditChangeProperty(PropertyChangeEvent);
  // auto name2 = PropertyChangeEvent.GetPropertyName();
  FName name = PropertyChangeEvent.MemberProperty
                   ? PropertyChangeEvent.MemberProperty->GetFName()
                   : NAME_None;
  UE_LOG(LogTemp, Log, TEXT("chick name: %s"), *name.ToString());

#define DOODLE_SET_VALUE(set_fun, value)                        \
  else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, value)) { \
    TArray<UDirectionalLightComponent*> out_list;               \
    this->GetComponents<UDirectionalLightComponent>(out_list);  \
    for (auto it = out_list.CreateIterator(); it; ++it) {       \
      (*it)->set_fun(value);                                    \
      (*it)->PostEditChangeProperty(PropertyChangeEvent);       \
    }                                                           \
  }

#define DOODLE_ASSIGN_VALUE(value)                              \
  else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, value)) { \
    TArray<UDirectionalLightComponent*> out_list;               \
    this->GetComponents<UDirectionalLightComponent>(out_list);  \
    for (auto it = out_list.CreateIterator(); it; ++it) {       \
      (*it)->value = value;                                     \
      (*it)->PostEditChangeProperty(PropertyChangeEvent);       \
    }                                                           \
  }

  if (name == GET_MEMBER_NAME_CHECKED(ThisClass, Intensity)) {
    TArray<UDirectionalLightComponent*> out_list;
    this->GetComponents<UDirectionalLightComponent>(out_list);
    for (auto it = out_list.CreateIterator(); it; ++it) {
      (*it)->SetIntensity(Intensity * (p_light_list.IsValidIndex(it.GetIndex()) ? p_light_list[it.GetIndex()] : 1));
      //(*it)->SetLightColor(LightColor);
      //(*it)->SetTemperature(Temperature);
      //(*it)->SetUseTemperature(bUseTemperature);
      //(*it)->LightSourceAngle = LightSourceAngle;
      //(*it)->LightSourceSoftAngle = LightSourceSoftAngle;
      //(*it)->bAffectsWorld = bAffectsWorld;
      //(*it)->SetCastShadows(CastShadows);
      //(*it)->SetLightingChannels(LightingChannels.bChannel0,
      //                           LightingChannels.bChannel1,
      //                           LightingChannels.bChannel2);
      //(*it)->SetAffectTranslucentLighting(bAffectTranslucentLighting);
      //(*it)->SetDynamicShadowDistanceMovableLight(
      //    DynamicShadowDistanceMovableLight);
      //(*it)->SetDynamicShadowDistanceStationaryLight(
      //    DynamicShadowDistanceStationaryLight);
      //(*it)->SetDynamicShadowCascades(DynamicShadowCascades);
      //(*it)->SetCascadeDistributionExponent(CascadeDistributionExponent);
      //(*it)->SetCascadeTransitionFraction(CascadeTransitionFraction);
      //(*it)->SetShadowDistanceFadeoutFraction(ShadowDistanceFadeoutFraction);
      //(*it)->bUseInsetShadowsForMovableObjects =
      //    bUseInsetShadowsForMovableObjects;
      //(*it)->FarShadowCascadeCount = FarShadowCascadeCount;
      //(*it)->FarShadowDistance = FarShadowDistance;
    }
  }
  DOODLE_SET_VALUE(SetLightColor, LightColor)
  DOODLE_SET_VALUE(SetTemperature, Temperature)
  DOODLE_SET_VALUE(SetUseTemperature, bUseTemperature)

  DOODLE_ASSIGN_VALUE(LightSourceAngle)
  DOODLE_ASSIGN_VALUE(LightSourceSoftAngle)
  DOODLE_ASSIGN_VALUE(bAffectsWorld)

  DOODLE_SET_VALUE(SetCastShadows, CastShadows)

  else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, LightingChannels)) {
    TArray<UDirectionalLightComponent*> out_list;
    this->GetComponents<UDirectionalLightComponent>(out_list);
    for (auto it = out_list.CreateIterator(); it; ++it) {
      (*it)->SetLightingChannels(LightingChannels.bChannel0, LightingChannels.bChannel1, LightingChannels.bChannel2);
      (*it)->PostEditChangeProperty(PropertyChangeEvent);
    }
  }

  DOODLE_SET_VALUE(SetAffectTranslucentLighting, bAffectTranslucentLighting)
  DOODLE_SET_VALUE(SetDynamicShadowDistanceMovableLight, DynamicShadowDistanceMovableLight)
  DOODLE_SET_VALUE(SetDynamicShadowDistanceStationaryLight, DynamicShadowDistanceStationaryLight)
  DOODLE_SET_VALUE(SetDynamicShadowCascades, DynamicShadowCascades)
  DOODLE_SET_VALUE(SetCascadeDistributionExponent, CascadeDistributionExponent)
  DOODLE_SET_VALUE(SetCascadeTransitionFraction, CascadeTransitionFraction)
  DOODLE_SET_VALUE(SetShadowDistanceFadeoutFraction, ShadowDistanceFadeoutFraction)
  DOODLE_ASSIGN_VALUE(bUseInsetShadowsForMovableObjects)
  DOODLE_ASSIGN_VALUE(FarShadowCascadeCount)
  DOODLE_ASSIGN_VALUE(FarShadowDistance)
}

#endif  // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
