#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ActorFactories/ActorFactory.h"

#include "ActorFactoryDoodleGeometryCache.generated.h"

class AActor;
struct FAssetData;

UCLASS(MinimalAPI, config = Editor)
class UActorFactoryDoodleGeometryCache :public UActorFactory
{
	GENERATED_UCLASS_BODY()
public:
	// 开始 UActorFactory 接口
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual void PostCreateBlueprint(UObject* Asset, AActor* CDO) override;
	// 结束 UActorFactory 接口
private:

};
