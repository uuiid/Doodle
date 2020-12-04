#pragma once 

#include "GeometryCache.h"

#include "DoodleAlemblcCacheAsset.generated.h"

UCLASS()
class DOODLE_API UDoodleAlemblcCacheAsset :public UGeometryCache
{
	GENERATED_BODY()
public:



	UPROPERTY(EditAnywhere, Category = DoodleCache)
	TArray<FString> materalName;
private:

};

