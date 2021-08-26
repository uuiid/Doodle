// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GeometryCacheComponent.h"
#include "DoodleGeometryCacheBones.h"

#include "DoodleCacheBonesComponent.generated.h"

//UCLASS(ClassGroup = (Rendering, Common), hidecategories = (Object, LOD), meta = (BlueprintSpawnableComponent), Experimental, ClassGroup = Experimental)

UCLASS(ClassGroup = (Common), hidecategories = (Object, LOD), meta = (BlueprintSpawnableComponent), Experimental, ClassGroup = Experimental)
class DOODLE_API UDoodleCacheBonesComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	// Sets default values for this component's properties
	UDoodleCacheBonesComponent();

	/*开始USceneComponent接口*/
	virtual bool             DoesSocketExist(FName InSocketName) const override;
	virtual FTransform       GetSocketTransform(FName InSocketNmae, ERelativeTransformSpace TransformSpace) const override;
	virtual bool             HasAnySockets() const override;
	virtual void             QuerySupportedSockets(TArray<FComponentSocketDescription>& qutSocket) const override;
	/*结束USceneComponent接口*/

	//使我们的函数每次调用
	//virtual bool ShouldTickIfViewportsOnly() const override { return true; };
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	UPROPERTY()
		UGeometryCacheComponent* p_GeometryCache_component;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "几何缓存骨骼引用")
		UDoodleGeometryCacheBones* p_GeometryCache_bones;


};
