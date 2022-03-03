// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleConfigLightActor.h"

#include "Animation/SkeletalMeshActor.h"
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

#define LOCTEXT_NAMESPACE "doodle"

ADoodleConfigLightActor::ADoodleConfigLightActor()
    : p_skin_mesh(), use_clear(true) {
  RootComponent = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
  RootComponent->SetMobility(EComponentMobility::Movable);
  // RootComponent->SetupAttachment(RootComponent);

  p_solt = "Root_M";
}

#if WITH_EDITOR

bool ADoodleConfigLightActor::OpenSaveDialog(const FString& InDefaultPath,
                                             const FString& InNewNameSuggestion,
                                             FString& OutPackageName) {
  FSaveAssetDialogConfig SaveAssetDialogConfig;
  {
    SaveAssetDialogConfig.DefaultPath = InDefaultPath;
    SaveAssetDialogConfig.DefaultAssetName = InNewNameSuggestion;
    SaveAssetDialogConfig.AssetClassNames.Add(
        UDoodleConfigLight::StaticClass()->GetFName());
    SaveAssetDialogConfig.ExistingAssetPolicy =
        ESaveAssetDialogExistingAssetPolicy::AllowButWarn;
    SaveAssetDialogConfig.DialogTitleOverride =
        LOCTEXT("SaveConfigPresetDialogTitle", "Save Doodle light Config");
  }

  FContentBrowserModule& ContentBrowserModule =
      FModuleManager::LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser");
  FString SaveObjectPath =
      ContentBrowserModule.Get().CreateModalSaveAssetDialog(
          SaveAssetDialogConfig);

  if (!SaveObjectPath.IsEmpty()) {
    OutPackageName = FPackageName::ObjectPathToPackageName(SaveObjectPath);
    return true;
  }

  return false;
}

UObject* ADoodleConfigLightActor::OpenDialog(
    const FString& InDefaultPath, const FString& InNewNameSuggestion) {
  FOpenAssetDialogConfig OpenAssetDialogConfig;
  {
    OpenAssetDialogConfig.DefaultPath = InDefaultPath;
    OpenAssetDialogConfig.AssetClassNames.Add(
        UDoodleConfigLight::StaticClass()->GetFName());
    OpenAssetDialogConfig.bAllowMultipleSelection = false;
    OpenAssetDialogConfig.DialogTitleOverride =
        LOCTEXT("OpenConfigPresetDialogTitle", "Open Doodle light Config");
  }

  FContentBrowserModule& ContentBrowserModule =
      FModuleManager::LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser");
  TArray<FAssetData> l_AssetData_ =
      ContentBrowserModule.Get().CreateModalOpenAssetDialog(
          OpenAssetDialogConfig);

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
  AssetToolsModule.Get().CreateUniqueAssetName(DialogStartPath / DefaultName,
                                               TEXT(""), UniquePackageName,
                                               UniqueAssetName);

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
#if ENGINE_MINOR_VERSION >= 26
  UPackage* NewPackage = CreatePackage(*NewPackageName);

#else if ENGINE_MINOR_VERSION < 26
  UPackage* NewPackage = CreatePackage(nullptr, *NewPackageName);
#endif

  UDoodleConfigLight* NewPreset = NewObject<UDoodleConfigLight>(
      NewPackage, UDoodleConfigLight::StaticClass(), *NewAssetName,
      RF_Public | RF_Standalone | RF_Transactional);
  if (NewPreset) {
    for (auto it = p_light_list.CreateIterator(); it; ++it) {
      if (it->light.IsValid()) {
        /// <summary>
        /// 这里要复制出去obj,
        /// 不可以使用关卡中的指针,要不然重新创建关卡时会内存泄露
        /// </summary>
        ALight* k_l = DuplicateObject(it->light.Get(), NewPackage);
        NewPreset->p_light.Add({k_l, it->weight});
      }
    }
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
  }
  if (use_clear) {
    for (auto it = p_light_list.CreateIterator(); it; ++it) {
      if (it->light.IsValid()) {
        GetWorld()->EditorDestroyActor(it->light.Get(), true);
      }
    }
    p_light_list.Empty();
  }

  for (auto it = l_config->p_light.CreateIterator(); it; ++it) {
    if (it->light) {
      auto ft = it->light->GetTransform();
      // UE_LOG(LogTemp, Log, TEXT("tran: %s"), *(ft.ToString()));
      FActorSpawnParameters k_t{};
      k_t.Template = it->light;
      // UE_LOG(LogTemp, Log, TEXT("rgb: %s"),
      //       *((*it)->GetLightColor().ToString()));
      FTransform k_f = it->light->GetTransform();
      ALight* k_a = GWorld->SpawnActor<ALight>(it->light->GetClass(), k_f, k_t);
      k_a->AttachToActor(this,
                         FAttachmentTransformRules::KeepRelativeTransform);
      p_light_list.Add({k_a, it->weight});
      // LoadObject<ALight>();
    }
  }

#endif  // WITH_EDITOR
}

#if WITH_EDITOR
void ADoodleConfigLightActor::PostEditChangeProperty(
    struct FPropertyChangedEvent& PropertyChangeEvent) {
  Super::PostEditChangeProperty(PropertyChangeEvent);
  // auto name2 = PropertyChangeEvent.GetPropertyName();
  auto name = PropertyChangeEvent.MemberProperty
                  ? PropertyChangeEvent.MemberProperty->GetFName()
                  : NAME_None;
  // UE_LOG(LogTemp, Log, TEXT("chick name: %s"), *(name.ToString()));
  // UE_LOG(LogTemp, Log, TEXT("chick MemberProperty: %s"),
  // *(name2.ToString()));

  if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_skin_mesh) ||
      name == GET_MEMBER_NAME_CHECKED(ThisClass, p_solt)) {
    if (p_skin_mesh && p_solt.IsValid() && !p_solt.IsNone())
      this->AttachToActor(
          p_skin_mesh, FAttachmentTransformRules::KeepWorldTransform, p_solt);
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_light_list)) {
    for (auto it = p_light_list.CreateIterator(); it; ++it) {
      if (it->light.IsValid()) {
        auto* l_p = it->light->GetParentActor();
        if (l_p != this) {
          if (l_p->GetClass() == ADoodleConfigLightActor::StaticClass()) {
            auto* l_p_config = Cast<ADoodleConfigLightActor>(l_p);
            if (l_p_config) {
              l_p_config->p_light_list.Remove(*it);
            }
          }

          it->light->AttachToActor(
              this, FAttachmentTransformRules::KeepWorldTransform);
        }
      }
    }
  }
}

#endif  // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
