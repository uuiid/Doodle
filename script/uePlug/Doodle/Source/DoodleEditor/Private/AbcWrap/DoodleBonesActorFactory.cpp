#include "AbcWrap/DoodleBonesActorFactory.h"

#include "AbcWrap/DoodleGeometryBonesActor.h"
#include "AbcWrap/DoodleGeometryCacheBones.h"
UDoodleBonesActorFactory::UDoodleBonesActorFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DisplayName = FText::FromString("Doodle Geometry Cache");
	NewActorClass = ADoodleGeometryBonesActor::StaticClass();
	bUseSurfaceOrientation = true;
}

bool UDoodleBonesActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (!AssetData.IsValid() || !(AssetData.GetClass() == UDoodleGeometryCacheBones::StaticClass()))
	{
		OutErrorMsg = FText::FromString("A valid GeometryCache must be specified.");
		return false;
	}
	return true;
}

void UDoodleBonesActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	UDoodleGeometryCacheBones* cache = CastChecked<UDoodleGeometryCacheBones>(Asset);

	// //更改一些属性
	ADoodleGeometryBonesActor* cacheActor = CastChecked<ADoodleGeometryBonesActor>(NewActor);
	cacheActor->p_GeoCache_bones->p_GeometryCache_bones = cache;

	cacheActor->p_GeoCache->UnregisterComponent();
	cacheActor->p_GeoCache->GeometryCache = cache->p_GeometryCache;
	cacheActor->p_GeoCache->RegisterComponent();

	cacheActor->p_GeoCache_bones->p_GeometryCache_component = cacheActor->p_GeoCache;
	//UDoodleGeometryCacheComponent * cacheComponent = cacheActor->GetGeometryCacheComponent();
	//check(cacheComponent);

   // cacheComponent->UnregisterComponent();
   // cacheComponent->GeometryCache = cache;

   // cacheComponent->RegisterComponent();
}

void UDoodleBonesActorFactory::PostCreateBlueprint(UObject* Asset, AActor* CDO)
{
	if (Asset != NULL && CDO != NULL)
	{
		// Set GeometryCache (data) instance
		UDoodleGeometryCacheBones* GeometryCache = CastChecked<UDoodleGeometryCacheBones>(Asset);
		ADoodleGeometryBonesActor* GeometryCacheActor = CastChecked<ADoodleGeometryBonesActor>(CDO);

		GeometryCacheActor->p_GeoCache_bones->p_GeometryCache_bones = GeometryCache;
	}
}
