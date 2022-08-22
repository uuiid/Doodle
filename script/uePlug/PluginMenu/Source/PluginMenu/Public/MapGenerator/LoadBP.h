#pragma once
#include "CoreMinimal.h"
#include "MapGenerator/DataType.h"

class FLoadBP
{
public:
	
	static TArray<USkeletalMesh*> FindCompatibleMeshes(UAnimSequence* AnimSequence);
	static TArray<FString> GetReferenceBP(FString& ProjectPath);
	static UClass* GetBPClass(FString& BPPackage);

	static TArray<UAnimSequence*> GetSkeletalMeshAnims(TArray<FAssetData> AnimAssetsData,TArray<TSharedPtr<FString>> LoadedBP);
	static TArray<TArray<UAnimSequence*>> GetBPAnims(TArray<FAssetData> AnimAssetsData, UClass* BPClass);

	static void AddBPActor(TArray<TArray<UAnimSequence*>> BPAnims, class ULevelSequence* ShotSequence, UClass* BPClass);
	static void AddBPActor(TArray<TArray<UAnimSequence*>> BPAnims,class ULevelSequence* ShotSequence,UClass* BPClass,int32 StartFrame);
	static void AddSkeletalMeshActor(ULevelSequence* ShotSequence, UAnimSequence* AnimSequence, int32 StartFrame);

	static void LoadSkeletonAnim(FString& SkeletonPackage, FString& ProjectPath, FString& MapPackage, FString& MapName, FString& LevelSequence, int32 StartFrame);
	static void LoadBP(FString& BPPackage, FString& ProjectPath, FString& MapPackage, FString& MapName, FString& LevelSequence,int32 StartFrame);
};
