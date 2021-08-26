// Fill out your copyright notice in the Description page of Project Settings.


#include "AbcWrap/DoodleGeometryBonesActor.h"

// Sets default values
ADoodleGeometryBonesActor::ADoodleGeometryBonesActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//auto rootComponent = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
	//rootComponent->SetMobility(EComponentMobility::Stationary);
	//rootComponent->SetupAttachment(RootComponent);


	p_GeoCache_bones = CreateDefaultSubobject<UDoodleCacheBonesComponent>(
		FName{ UDoodleCacheBonesComponent::StaticClass()->GetName() });

	p_GeoCache_bones->SetMobility(EComponentMobility::Stationary);
	SetRootComponent(p_GeoCache_bones);

	p_GeoCache = CreateDefaultSubobject<UGeometryCacheComponent>(
		FName{ UGeometryCacheComponent::StaticClass()->GetName() }
	);
}

// Called when the game starts or when spawned
void ADoodleGeometryBonesActor::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ADoodleGeometryBonesActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

