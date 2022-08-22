#pragma once
#include "CoreMinimal.h"

#include "MapGenerator/DataType.h"

class USkeletalMesh;
class UAnimSequence;

class FLoadAnimationUtils
{
public:
	static TArray<USkeletalMesh*> FindCompatibleMeshes(UAnimSequence* AnimSequence);
};