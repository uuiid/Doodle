// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DoodleVariantObject.generated.h"

USTRUCT()
struct FVariantInfo
{   GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Materials")
	TArray<FSkeletalMaterial> Variants;
};
/**
 * 
 */
UCLASS()
class DOODLEEDITOR_API UDoodleVariantObject : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "SkeletalMesh")
	USkeletalMesh* Mesh;
	UPROPERTY(EditAnywhere,Category = "Varaints")
	TMap<FString, FVariantInfo> AllVaraint;
};
