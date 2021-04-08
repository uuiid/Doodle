// Fill out your copyright notice in the Description page of Project Settings.


#include "AbcWrap/DoodleGeometryCacheBones.h"

#include "GeometryCacheComponent.h"
#include "GeometryCache.h"
// Sets default values for this component's properties
UDoodleGeometryCacheBones::UDoodleGeometryCacheBones(const FObjectInitializer& ObjectInitialize)
	:Super(ObjectInitialize),
	p_GeometryCache(),
	p_GeometryCache_curve()
{
	//// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	//// off to improve performance if you don't need them.
	//PrimaryComponentTick.bCanEverTick = true;

	// ...

	this->p_GeometryCache = /*NewObject<UGeometryCache>(this);*/
		this->CreateDefaultSubobject<UGeometryCache>(FName{ UGeometryCache::StaticClass()->GetName() });
	this->p_GeometryCache_curve = this->CreateDefaultSubobject<UDoodleGeometryCacheCurveAsset>(FName{ UDoodleGeometryCacheCurveAsset::StaticClass()->GetName() });
}


