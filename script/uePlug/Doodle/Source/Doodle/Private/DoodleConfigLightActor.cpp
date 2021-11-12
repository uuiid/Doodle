// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleConfigLightActor.h"

#include "Animation/SkeletalMeshActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "DoodleConfigLight.h"
#include "Engine/Light.h"
#include "Engine/World.h"
#include "IContentBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "doodle"

ADoodleConfigLightActor::ADoodleConfigLightActor()
    : p_skin_mesh(), p_light(), use_clear(true) {
  auto rootComponent =
      CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
  rootComponent->SetMobility(EComponentMobility::Stationary);
  rootComponent->SetupAttachment(RootComponent);
  SetRootComponent(rootComponent);

  p_light =
      CreateDefaultSubobject<UDoodleConfigLight>(FName{"UDoodleConfigLight"});
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
  UPackage* NewPackage = CreatePackage(nullptr, *NewPackageName);

  UDoodleConfigLight* NewPreset = NewObject<UDoodleConfigLight>(
      NewPackage, UDoodleConfigLight::StaticClass(), *NewAssetName,
      RF_Public | RF_Standalone | RF_Transactional);
  if (NewPreset) {
    for (auto it = p_light_list.CreateIterator(); it; ++it) {
      if ((*it).IsValid()) {
        /// <summary>
        /// 这里要复制出去obj,
        /// 不可以使用关卡中的指针,要不然重新创建关卡时会内存泄露
        /// </summary>
        ALight* k_l = DuplicateObject((*it).Get(), NewPackage);
        NewPreset->p_light.Add(k_l);
      }
    }
    NewPreset->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewPreset);

    p_light = NewPreset;
  }
#endif  // WITH_EDITOR
}

#if WITH_EDITOR

void ADoodleConfigLightActor::PostEditChangeProperty(
    struct FPropertyChangedEvent& PropertyChangeEvent) {
  Super::PostEditChangeProperty(PropertyChangeEvent);
  auto name2 = PropertyChangeEvent.GetPropertyName();
  auto name = PropertyChangeEvent.MemberProperty
                  ? PropertyChangeEvent.MemberProperty->GetFName()
                  : NAME_None;
  UE_LOG(LogTemp, Log, TEXT("chick name: %s"), *(name.ToString()));
  UE_LOG(LogTemp, Log, TEXT("chick MemberProperty: %s"), *(name2.ToString()));

  if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_light)) {
    if (!p_light) return;
    if (use_clear) {
      for (auto it = p_light_list.CreateIterator(); it; ++it) {
        if ((*it).IsValid()) {
          (*it)->Destroy();
        }
      }
      p_light_list.Empty();
    }

    for (auto it = p_light->p_light.CreateIterator(); it; ++it) {
      if (*it) {
        auto ft = (*it)->GetTransform();
        // UE_LOG(LogTemp, Log, TEXT("tran: %s"), *(ft.ToString()));
        FActorSpawnParameters k_t{};
        k_t.Template = *it;
        // UE_LOG(LogTemp, Log, TEXT("rgb: %s"),
        //       *((*it)->GetLightColor().ToString()));
        FTransform k_f = (*it)->GetTransform();
        ALight* k_a = GWorld->SpawnActor<ALight>((*it)->GetClass(), k_f, k_t);
        k_a->AttachToActor(this,
                           FAttachmentTransformRules::KeepRelativeTransform);
        p_light_list.Add(k_a);
        // LoadObject<ALight>();
      }
    }
    // UE_LOG(LogTemp, Log, TEXT("set property: %s"), *(name.ToString()));
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_skin_mesh) ||
             name == GET_MEMBER_NAME_CHECKED(ThisClass, p_solt)) {
    if (p_skin_mesh && p_solt.IsValid() && !p_solt.IsNone())
      this->AttachToActor(
          p_skin_mesh, FAttachmentTransformRules::KeepWorldTransform, p_solt);
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_light_list)) {
    for (auto it = p_light_list.CreateIterator(); it; ++it) {
      if ((*it).IsValid()) {
        if ((*it)->GetParentActor() != this) {
          (*it)->AttachToActor(this,
                               FAttachmentTransformRules::KeepWorldTransform);
        }
      }
    }
  }
}

#endif  // WITH_EDITOR