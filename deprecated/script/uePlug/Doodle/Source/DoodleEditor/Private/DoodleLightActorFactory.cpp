#include "DoodleLightActorFactory.h"

#include "DoodleConfigLight.h"
#include "DoodleConfigLightActor.h"

UDoodleLightActorFactory::UDoodleLightActorFactory(
    const FObjectInitializer& ObjectInitializer
)
    : Super(ObjectInitializer) {
  DisplayName            = FText::FromString("Doodle Light config");
  NewActorClass          = ADoodleConfigLightActor::StaticClass();
  bUseSurfaceOrientation = true;
}

bool UDoodleLightActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) {
  if (!AssetData.IsValid() ||
      !(AssetData.GetClass() == UDoodleConfigLight::StaticClass())) {
    OutErrorMsg = FText::FromString("A valid GeometryCache must be specified.");
    return false;
  }
  return true;
}

// void UDoodleLightActorFactory::PostSpawnActor(UObject* Asset,
//                                              AActor* NewActor) {
//  Super::PostSpawnActor(Asset, NewActor);
//
//  // UDoodleConfigLight* cache = CastChecked<UDoodleConfigLight>(Asset);
//
//  // UDoodleConfigLight* cacheActor =
//  CastChecked<UDoodleConfigLight>(NewActor);
//  // UDoodleGeometryCacheComponent * cacheComponent =
//  // cacheActor->GetGeometryCacheComponent(); check(cacheComponent);
//
//  // cacheComponent->UnregisterComponent();
//  // cacheComponent->GeometryCache = cache;
//
//  // cacheComponent->RegisterComponent();
//}

AActor* UDoodleLightActorFactory::SpawnActor(
    UObject* InAsset, ULevel* InLevel, const FTransform& InTransform,
    const FActorSpawnParameters& InSpawnParams
) {
  ULevel* LocalLevel = ValidateSpawnActorLevel(InLevel, InSpawnParams);

  //  AActor* DefaultActor = GetDefaultActor(FAssetData(InAsset));
  //  if ((DefaultActor != nullptr) && (LocalLevel != nullptr)) {
  //    FActorSpawnParameters SpawnParamsCopy(InSpawnParams);
  //    SpawnParamsCopy.OverrideLevel = LocalLevel;
  //    SpawnParamsCopy.bCreateActorPackage = true;
  // #if WITH_EDITOR
  //    SpawnParamsCopy.bTemporaryEditorActor =
  //        FLevelEditorViewportClient::IsDroppingPreviewActor();
  // #endif
  //    return LocalLevel->OwningWorld->SpawnActor(DefaultActor->GetClass(),
  //                                               &InTransform,
  //                                               SpawnParamsCopy);
  //  }
  //
  //  return NULL;

  AActor* NewActor   = nullptr;

  UObject* l_obj     = FAssetData{InAsset}.FastGetAsset();

  auto* l_config     = Cast<UDoodleConfigLight>(l_obj);
  if (l_config) {
    FActorSpawnParameters l_par{InSpawnParams};

    l_par.Template = l_config->p_Actor;
    NewActor       = Super::SpawnActor(InAsset, InLevel, InTransform, l_par);
    //  NewActor = LocalLevel->OwningWorld->SpawnActor<ADoodleConfigLightActor>(
    //      ADoodleConfigLightActor::StaticClass(), InTransform, l_par);
  }

  //{
  //}
  return NewActor;
}

AActor* UDoodleLightActorFactory::GetDefaultActor(const FAssetData& AssetData) {
  return ADoodleConfigLightActor::StaticClass()->GetDefaultObject<AActor>();
}
