// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Animation/AnimCurveTypes.h"

#include "DoodleGeometryCacheCurveAsset.generated.h"

//USTRUCT()
//struct DOODLE_API FDoodleAnimation :public FTransformCurve {
//	GENERATED_USTRUCT_BODY();
//
//	FDoodleAnimation(FSmartName InName, int32 InCurveTypeFlags)
//		: FTransformCurve(InName, InCurveTypeFlags)
//	{
//	};
//	FDoodleAnimation() {};
//
//	bool operator == (const FName& k_name) const {
//		return Name.DisplayName == k_name;
//	};
//};

// This class does not need to be modified.
UCLASS(hidecategories = Object, BlueprintType, config = Engine)
class DOODLE_API UDoodleGeometryCacheCurveAsset : public UObject
{
	//GENERATED_BODY()
	GENERATED_UCLASS_BODY()
public:
	TArray<FTransformCurve> &GetTranAnm();
	//UDoodleGeometryCacheCurveAsset();

	UPROPERTY(EditAnywhere, Category = doodle)
	TArray<FString> materalName;

	UPROPERTY()
	TArray<FTransformCurve> tranAnm;
};
