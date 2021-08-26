// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbcWrap/DoodleGeometryCacheBones.h"
#include "AbcWrap/DoodleCacheBonesComponent.h"

#include "DoodleGeometryBonesActor.generated.h"

UCLASS(hidecategories = Object, BlueprintType, config = Engine)
class DOODLE_API ADoodleGeometryBonesActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADoodleGeometryBonesActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "doodle几何缓存骨骼引用")
	UDoodleCacheBonesComponent *p_GeoCache_bones;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "几何缓存")
	UGeometryCacheComponent *p_GeoCache;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
