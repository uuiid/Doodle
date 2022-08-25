#pragma once
#include "CoreMinimal.h"

class FConvertPath {
 public:
  static FString ToRelativePath(FString& AbsolutePath);
  static FString ToAbsolutePath(FString& RelativePath);

  static FString ToRelativePath(FString& AbsolutePath, bool WithExt);
  static FString ToAbsolutePath(FString& RelativePath, bool WithExt, FString& Ext);

  static FString GetPackageName(FString& Package);
  static FString GetPackagePath(FString& Package);
};
