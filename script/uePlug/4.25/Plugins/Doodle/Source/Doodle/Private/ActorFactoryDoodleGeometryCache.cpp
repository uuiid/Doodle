#include "ActorFactoryDoodleGeometryCache.h"

#include "DoodleGeometryCacheActor.h"
#include "DoodleAlemblcCacheAsset.h"

#include "DoodleGeometryCacheComponent.h"

UActorFactoryDoodleGeometryCache::UActorFactoryDoodleGeometryCache(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DisplayName = FText::FromString("Geometry Cache");
	NewActorClass = ADoodleGeometryCacheActor::StaticClass();
	bUseSurfaceOrientation = true;
}

bool UActorFactoryDoodleGeometryCache::CanCreateActorFrom(const FAssetData & AssetData, FText & OutErrorMsg)
{
	if (!AssetData.IsValid() || !AssetData.GetClass()->IsChildOf(UDoodleAlemblcCache::StaticClass()))
	{
		OutErrorMsg = FText::FromString("A valid GeometryCache must be specified.");
		return false;
	}

	return true;
}

void UActorFactoryDoodleGeometryCache::PostSpawnActor(UObject * Asset, AActor * NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	UDoodleAlemblcCache* cache = CastChecked<UDoodleAlemblcCache>(Asset);

	//更改一些属性
	ADoodleGeometryCacheActor* cacheActor = CastChecked<ADoodleGeometryCacheActor>(NewActor);
	UDoodleGeometryCacheComponent * cacheComponent = cacheActor->GetGeometryCacheComponent();
	check(cacheComponent);

	cacheComponent->UnregisterComponent();
	cacheComponent->GeometryCache = cache;

	cacheComponent->RegisterComponent();
}

void UActorFactoryDoodleGeometryCache::PostCreateBlueprint(UObject * Asset, AActor * CDO)
{
	if (Asset != NULL && CDO != NULL)
	{
		// Set GeometryCache (data) instance
		UDoodleAlemblcCache* GeometryCache = CastChecked<UDoodleAlemblcCache>(Asset);
		ADoodleGeometryCacheActor* GeometryCacheActor = CastChecked<ADoodleGeometryCacheActor>(CDO);
		UDoodleGeometryCacheComponent* GeometryCacheComponent = GeometryCacheActor->GetGeometryCacheComponent();

		GeometryCacheComponent->GeometryCache = GeometryCache;
	}
}
