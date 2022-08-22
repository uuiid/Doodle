#pragma once
#include "CoreMinimal.h"
#include "MapGenerator/DataType.h"


class FCreateMap
{
public:
	static void CreateMap(FString& ProjectPath, FString& FbxCameraDir,FString& Shot,FString& LevelMap, FString& LevelStreamingMap,bool IsSaveInMap);
	static void CreateBPMap(FString& ProjectPath, FString& FbxCameraDir, FString& Shot, FString& LevelMap, FString& LevelStreamingMap, bool IsSaveInMap,TArray<TSharedPtr<FString>> LoadedBP);
	static void CreateBPMap(FString& ProjectPath, FString& FbxCameraDir, FString& Shot, FString& LevelMap, FString& LevelStreamingMap, bool IsSaveInMap, TArray<TSharedPtr<struct FBPInfo>> AllBPInfo);
};
