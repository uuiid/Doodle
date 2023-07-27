#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;
class UGeometryCache;
class UAnimSequence;
struct FDoodleUSkeletonData_1;

class FDoodleCreateLevel {
  TArray<FDoodleUSkeletonData_1> AllSkinObjs;
  void ImportCamera(const FString& InFbxpath);
  void ImportSkeletalMesh(const FString& InFbxpath);
  void ImportGeometryCache(const FString& InAbcPath);

  void ImportFile(const FString& InFile);

 public:
  void ImportFiles(const TArray<FString>& InFiles);
};
