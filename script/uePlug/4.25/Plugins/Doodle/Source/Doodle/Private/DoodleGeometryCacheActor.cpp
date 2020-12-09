#include "DoodleGeometryCacheActor.h"

ADoodleGeometryCacheActor::ADoodleGeometryCacheActor(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer) {
	GeometryCacheComponent = CreateDefaultSubobject<UDoodleGeometryCacheComponent>(TEXT("GeometryCacheComponent"));
	RootComponent = GeometryCacheComponent;
};

UDoodleGeometryCacheComponent * ADoodleGeometryCacheActor::GetGeometryCacheComponent() const
{
	return GeometryCacheComponent;
}
