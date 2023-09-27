// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DoodleVariantObject.generated.h"

USTRUCT()
struct FDATA
{   GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere)
	TArray<FSkeletalMaterial> varaints;
};
/**
 * 
 */
UCLASS()
class DOODLEEDITOR_API UDoodleVariantObject : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, DisplayName = "")
	USkeletalMesh* mesh;
	UPROPERTY(VisibleAnywhere,DisplayName = "lu_jin")
	FName path;
	UPROPERTY(EditAnywhere,DisplayName = "bian_ti")
	TMap<FString,FDATA> all_varaint;
};
