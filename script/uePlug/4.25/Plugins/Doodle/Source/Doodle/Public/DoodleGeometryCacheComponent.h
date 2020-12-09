#pragma once

#include "GeometryCacheComponent.h"

#include "DoodleGeometryCacheComponent.generated.h"


UCLASS(ClassGroup = (Rendering, Common), hidecategories = (Object, LOD), meta = (BlueprintSpawnableComponent), Experimental, ClassGroup = Experimental)
class UDoodleGeometryCacheComponent :public UGeometryCacheComponent
{
	GENERATED_UCLASS_BODY()
public:
	/** Required for access to (protected) TrackSections */
	friend class FGeometryCacheSceneProxy;

	/*开始USceneComponent接口*/
	virtual bool             DoesSocketExist         (FName InSocketName) const override;
	virtual FTransform       GetSocketTransform      (FName InSocketNmae, ERelativeTransformSpace TransformSpace) const override;
	virtual bool             HasAnySockets           () const override;
	virtual void             QuerySupportedSockets   (TArray<FComponentSocketDescription>& qutSocket) const override;
	/*结束USceneComponent接口*/

	/*开始UActorComponent接口*/
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	/*结束UActorComponent接口*/

	/*开始geoCache接口*/
	void TickAtThisTime(const float Time, bool bInIsRunning, bool bInBackwards, bool bInIsLooping);
	/*结束geoCache接口*/
private:

};