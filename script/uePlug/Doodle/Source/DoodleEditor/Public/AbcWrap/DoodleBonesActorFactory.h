#pragma	once


#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ActorFactories/ActorFactory.h"

#include "DoodleBonesActorFactory.generated.h"



UCLASS(MinimalAPI, config = Editor)
class UDoodleBonesActorFactory : public UActorFactory
{
	GENERATED_UCLASS_BODY()


	// 开始 UActorFactory 接口
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual void PostCreateBlueprint(UObject* Asset, AActor* CDO) override;
	// 结束 UActorFactory 接口

};