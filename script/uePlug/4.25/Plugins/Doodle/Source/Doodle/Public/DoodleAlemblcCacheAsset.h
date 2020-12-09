#pragma once 

#include "GeometryCache.h"

#include "DoodleAlemblcCacheAsset.generated.h"

//USTRUCT()
//struct FDoodleGeometryCache {
//	GENERATED_USTRUCT_BODY()
//
//	FDoodleGeometryCache() :MaterialInterface(nullptr), MaterialSlotName(NAME_None) {};
//	FDoodleGeometryCache(
//		class UMaterialInterface* InMaterialInterface,
//		FName InMaterialSlotName = NAME_None)
//		:MaterialInterface(InMaterialInterface),
//		MaterialSlotName(InMaterialSlotName) {};
//
//	friend FArchive & operator << (FArchive& Ar, FDoodleGeometryCache &Elem) {};
//
//	UPROPERTY(EditAnywhere, Category = GeometryCache)
//	class UMaterialInterface *	MaterialInterface;
//
//	UPROPERTY(EditAnywhere, Category = GeometryCache)
//	FName MaterialSlotName;
//};

USTRUCT()
struct DOODLE_API FDoodleAnimation :public FTransformCurve {
	GENERATED_USTRUCT_BODY();

	FDoodleAnimation(FSmartName InName, int32 InCurveTypeFlags)
		: FTransformCurve(InName, InCurveTypeFlags)
	{
	};
	FDoodleAnimation() {};

	bool operator == (const FName & k_name) const {
		return Name.DisplayName == k_name;
	};
};


class USkeleton;

UCLASS()
class DOODLE_API UDoodleAlemblcCache :public UGeometryCache
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(EditAnywhere, Category = doodle)
	TArray<FString> materalName;


	//UPROPERTY(EditAnywhere, Category = doodle)
	//USkeleton  * skeleton;
	
	UPROPERTY()
	TArray<FDoodleAnimation> tranAnm;

	//FDoodleAnimation FindDoodleAnimation(const FName &k_name);
	//UPROPERTY(EditAnywhere, Category = GeometryCache)
	//TArray<FDoodleGeometryCache > materalList;
private:

};

