
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GameFramework/Actor.h"

#include "DoodleGeometryCacheActor.generated.h"

class UDoodleGeometryCacheComponent;

UCLASS()
class ADoodleGeometryCacheActor : public AActor
{
	GENERATED_UCLASS_BODY()
public:
	
	//virtual bool GetReferencedContentObjects(TArray<UObject*>& Objects) const override;
	//只有一个返回组件的方法就行了
	UFUNCTION(BlueprintCallable, Category = "Components|GeometryCache")
	UDoodleGeometryCacheComponent* GetGeometryCacheComponent() const;
private:
	UPROPERTY(Category = GeometryCacheActor, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|GeometryCache", AllowPrivateAccess = "true"))
	UDoodleGeometryCacheComponent* GeometryCacheComponent;
};
