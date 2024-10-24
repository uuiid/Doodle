#pragma once

#include "CoreMinimal.h"

// #include "DoodleCreateLevel.generated.h"
#include "Doodle/DoodleImportFbxUI.h"

class USkeletalMesh;
class UGeometryCache;
class UAnimSequence;
struct FDoodleUSkeletonData_1;
class UDoodleBaseImportData;
class FDoodleCreateLevel : public FGCObject {
  TArray<FDoodleUSkeletonData_1> AllSkinObjs;
  void ImportCamera(const FString& InFbxpath);
  void ImportSkeletalMesh(const FString& InFbxpath);
  void ImportGeometryCache(const FString& InAbcPath);

  void ImportFile(const FString& InFile);
  void PreparationImport(const FString& InFiles);

  TArray<TObjectPtr<UDoodleBaseImportData>> AllImportData;

 public:
  void ImportFiles(const TArray<FString>& InFiles);

  void AddReferencedObjects(FReferenceCollector& Collector) override;
  FString GetReferencerName() const override;
};
