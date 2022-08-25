// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ActorFactories/ActorFactory.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

///
///
#include "DoodleLightActorFactory.generated.h"
UCLASS(MinimalAPI, config = Editor)
class UDoodleLightActorFactory : public UActorFactory {
  GENERATED_UCLASS_BODY()

  // 开始 UActorFactory 接口
  virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
  // virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
  virtual AActor* SpawnActor(
      UObject* InAsset, ULevel* InLevel, const FTransform& InTransform,
      const FActorSpawnParameters& InSpawnParams
  ) override;
  AActor* GetDefaultActor(const FAssetData& AssetData) override;

  // 结束 UActorFactory 接口
};
