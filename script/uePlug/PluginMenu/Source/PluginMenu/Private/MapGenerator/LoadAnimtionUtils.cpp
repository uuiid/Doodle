#include "MapGenerator/LoadAnimationUtils.h"

#include "UObject/UObjectGlobals.h"
#include "Engine.h"
#include "DesktopPlatform/Public/IDesktopPlatform.h"
#include "DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Editor/UnrealEd/Public/EditorDirectories.h"
#include "AssetRegistryModule.h"
#include "FileHelpers.h"
#include "AssetData.h"

#include "LevelSequence.h"


#include "Animation/SkeletalMeshActor.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "MovieSceneTracks/Public/Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "MovieSceneTracks/Public/Tracks/MovieScene3DTransformTrack.h"
#include "MovieSceneTracks/Public/Sections/MovieScene3DTransformSection.h"

#define LOCTEXT_NAMESPACE "FLoadAnimationUtils"

TArray<USkeletalMesh*> FLoadAnimationUtils::FindCompatibleMeshes(UAnimSequence* AnimSequence)
{
	TArray<USkeletalMesh*> SkeletalMeshes;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FARFilter Filter;
	Filter.ClassNames.Add(USkeletalMesh::StaticClass()->GetFName());

	FString SkeletonString = FAssetData(AnimSequence->GetSkeleton()).GetExportTextName();
	Filter.TagsAndValues.Add(GET_MEMBER_NAME_CHECKED(USkeletalMesh, Skeleton), SkeletonString);

	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);


	if (AssetList.Num() > 0)
	{
		for (auto Asset : AssetList)
		{
			USkeletalMesh * SkeletalMesh = Cast<USkeletalMesh>(Asset.GetAsset());
			SkeletalMeshes.Add(SkeletalMesh);
		}
	}
	return SkeletalMeshes;

}

