// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GeometryCacheComponent.h"
#include "AbcWrap/DoodleGeometryCacheCurveAsset.h"

#include "DoodleGeometryCacheBones.generated.h"


UCLASS(hidecategories = Object, BlueprintType, config = Engine)
class DOODLE_API UDoodleGeometryCacheBones : public UObject, public IInterface_AssetUserData
{
	GENERATED_UCLASS_BODY()

public:


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "几何缓存骨骼引用")
		UGeometryCache* p_GeometryCache;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "几何缓存骨骼引用")
		UDoodleGeometryCacheCurveAsset* p_GeometryCache_curve;
};
