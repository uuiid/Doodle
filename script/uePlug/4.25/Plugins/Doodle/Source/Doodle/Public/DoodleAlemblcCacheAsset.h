#pragma once 

#include "GeometryCache.h"

#include "DoodleAlemblcCacheAsset.generated.h"

UCLASS()
class DOODLE_API UDoodleAlemblcCacheAsset :public UGeometryCache
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY()
	TArray<FString> materalName;
private:

};

