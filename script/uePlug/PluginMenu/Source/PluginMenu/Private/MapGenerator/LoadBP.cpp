#include "MapGenerator/LoadBP.h"

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

#ifdef LOCTEXT_NAMESPACE
#undef LOCTEXT_NAMESPACE
#endif

#define LOCTEXT_NAMESPACE "FLoadBP"

UClass *FLoadBP::GetBPClass(FString &BPPackage)
{
	TArray<FString> BPNameSplit;
	BPPackage.ParseIntoArray(BPNameSplit, TEXT("/"), true);
	if (BPNameSplit.Num())
	{
		FString BPClassName = BPPackage + "." + BPNameSplit[BPNameSplit.Num() - 1] + "_C";
		UClass *BPClass = LoadClass<AActor>(NULL, *BPClassName);
		if (BPClass != nullptr)
		{
			return BPClass;
		}
	}

	return nullptr;
}

TArray<USkeletalMesh *> FLoadBP::FindCompatibleMeshes(UAnimSequence *AnimSequence)
{
	TArray<USkeletalMesh *> SkeletalMeshes;

	FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FARFilter Filter;
	Filter.ClassNames.Add(USkeletalMesh::StaticClass()->GetFName());

	FString SkeletonString = FAssetData(AnimSequence->GetSkeleton()).GetExportTextName();
	Filter.TagsAndValues.Add(TEXT("Skeleton"), SkeletonString);

	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() > 0)
	{
		for (auto Asset : AssetList)
		{
			USkeletalMesh *SkeletalMesh = Cast<USkeletalMesh>(Asset.GetAsset());
			SkeletalMeshes.Add(SkeletalMesh);
		}
	}
	return SkeletalMeshes;
}

void FLoadBP::AddBPActor(TArray<TArray<UAnimSequence *>> BPAnims, ULevelSequence *ShotSequence, UClass *BPCLass)
{
	TArray<UAnimSequence *> RootAnim;
	int32 max = 0;

	if (BPAnims.Num())
	{
		for (auto Anims : BPAnims)
		{
			if (Anims.Num() > max)
			{
				max = Anims.Num();
				RootAnim = Anims;
			}
		}

		for (int32 i = 0; i < RootAnim.Num(); i++)
		{
			AActor *BPActor = GWorld->SpawnActor<AActor>(BPCLass);
			// BPActor->SetActorLabel(RootAnim[i]->GetName() + "_" + FString::FromInt(i));
			TArray<USkeletalMeshComponent *> BPComponents{};
			BPActor->GetComponents<USkeletalMeshComponent>(BPComponents);

			UMovieScene *MyMovieScene = ShotSequence->GetMovieScene();
			FFrameNumber MySequenceStart = MyMovieScene->GetTickResolution().Numerator * 71 / 25;
			FGuid BPActorGuid = MyMovieScene->AddPossessable(BPActor->GetActorLabel(), BPActor->GetClass());
			ShotSequence->BindPossessableObject(BPActorGuid, *BPActor, BPActor->GetWorld());

			for (int32 j = 0; j < BPComponents.Num(); j++)
			{

				USkeletalMeshComponent *SkeletalMeshComponent = (USkeletalMeshComponent *)BPComponents[j];
				FGuid BPActorComponentGuid = MyMovieScene->AddPossessable(BPComponents[j]->GetFName().ToString(), BPComponents[j]->GetClass());
				ShotSequence->BindPossessableObject(BPActorComponentGuid, *BPComponents[j], BPActor->GetWorld());
				MyMovieScene->FindPossessable(BPActorComponentGuid)->SetParent(BPActorGuid);
				UMovieSceneSkeletalAnimationTrack *AnimTrack = MyMovieScene->AddTrack<UMovieSceneSkeletalAnimationTrack>(BPActorComponentGuid);
				if (BPAnims[j].IsValidIndex(i))
					AnimTrack->AddNewAnimation(MySequenceStart, (UAnimSequenceBase *)BPAnims[j][i]);
			}
		}
	}
}

void FLoadBP::AddBPActor(TArray<TArray<UAnimSequence *>> BPAnims, ULevelSequence *ShotSequence, UClass *BPCLass, int32 StartFrame)
{
	TArray<UAnimSequence *> RootAnim;
	int32 max = 0;

	if (BPAnims.Num())
	{
		for (auto Anims : BPAnims)
		{
			if (Anims.Num() > max)
			{
				max = Anims.Num();
				RootAnim = Anims;
			}
		}

		for (int32 i = 0; i < RootAnim.Num(); i++)
		{
			AActor *BPActor = GWorld->SpawnActor<AActor>(BPCLass);
			// BPActor->SetActorLabel(RootAnim[i]->GetName() + "_" + FString::FromInt(i));
			TArray<USkeletalMeshComponent *> BPComponents;
			BPActor->GetComponents<USkeletalMeshComponent>(BPComponents);

			UMovieScene *MyMovieScene = ShotSequence->GetMovieScene();
			FFrameNumber MySequenceStart = MyMovieScene->GetTickResolution().Numerator * StartFrame / 25;
			FGuid BPActorGuid = MyMovieScene->AddPossessable(BPActor->GetActorLabel(), BPActor->GetClass());
			ShotSequence->BindPossessableObject(BPActorGuid, *BPActor, BPActor->GetWorld());

			for (int32 j = 0; j < BPComponents.Num(); j++)
			{

				USkeletalMeshComponent *SkeletalMeshComponent = BPComponents[j];
				FGuid BPActorComponentGuid = MyMovieScene->AddPossessable(BPComponents[j]->GetFName().ToString(), BPComponents[j]->GetClass());
				ShotSequence->BindPossessableObject(BPActorComponentGuid, *BPComponents[j], BPActor->GetWorld());
				MyMovieScene->FindPossessable(BPActorComponentGuid)->SetParent(BPActorGuid);
				UMovieSceneSkeletalAnimationTrack *AnimTrack = MyMovieScene->AddTrack<UMovieSceneSkeletalAnimationTrack>(BPActorComponentGuid);
				if (BPAnims[j].IsValidIndex(i))
					AnimTrack->AddNewAnimation(MySequenceStart, (UAnimSequenceBase *)BPAnims[j][i]);
			}
		}
	}
}

TArray<FString> FLoadBP::GetReferenceBP(FString &ProjectPath)
{
	TArray<FString> ReferenceBP;

	// Get SubDirectories In ProjectPath
	TArray<FString> SubDirs;
	FString FinalPath = FPaths::ProjectContentDir() + "/" + ProjectPath + "/" + TEXT("*");
	IFileManager::Get().FindFiles(SubDirs, *FinalPath, false, true);

	UObjectLibrary *AnimLibrary = UObjectLibrary::CreateLibrary(UAnimSequence::StaticClass(), false, GIsEditor);
	if (AnimLibrary)
	{
		AnimLibrary->AddToRoot();
	}

	for (auto ShotDir : SubDirs)
	{
		// Get Anim Assets In Shot Directory
		if (ShotDir.StartsWith("SC", ESearchCase::IgnoreCase))
		{
			FString AnimAssetsPath = "/Game/" + ProjectPath + "/" + ShotDir + "/Anim";
			AnimLibrary->LoadAssetDataFromPath(*AnimAssetsPath);

			TArray<FAssetData> AnimAssetsData;
			AnimLibrary->GetAssetDataList(AnimAssetsData);

			if (AnimAssetsData.Num())
			{
				// Get Skeletal Meshes for Each Anim Sequence
				TArray<USkeletalMesh *> AnimSkeletalMeshs;
				TArray<USkeleton *> SearchedSkeleton;

				for (auto Anim : AnimAssetsData)
				{
					FString AnimSoftPath = Anim.ObjectPath.ToString();
					UAnimSequence *AnimSequence = LoadObject<UAnimSequence>(NULL, *AnimSoftPath);

					USkeleton *AnimSkeleton = AnimSequence->GetSkeleton();
					int32 FindIndex;
					if (!SearchedSkeleton.Find(AnimSkeleton, FindIndex))
					{

						AnimSkeletalMeshs = FLoadBP::FindCompatibleMeshes(AnimSequence);
						if (AnimSkeletalMeshs.Num())
							for (auto SkeletalMesh : AnimSkeletalMeshs)
							{
								// Get Skeletal Mesh PackageName
								TArray<FString> NameSplit;
								SkeletalMesh->GetPathName().ParseIntoArray(NameSplit, TEXT("."), true);
								FName SkeletalMeshPackageName = FName(*NameSplit[0]);

								// Get Hard Refecencers for Each Skeletal Mesh
								TArray<FName> HardReferencers;
								FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
								AssetRegistryModule.Get().GetReferencers(
									SkeletalMeshPackageName,
									HardReferencers,
									UE::AssetRegistry::EDependencyCategory::All // EAssetRegistryDependencyType::Hard
								);

								// Check Hard Referencer is BP Asset
								if (HardReferencers.Num())
								{
									for (auto Referencer : HardReferencers)
									{
										FString BPPackage = Referencer.ToString();
										UClass *BPClass = FLoadBP::GetBPClass(BPPackage);
										if (BPClass != nullptr)
											ReferenceBP.AddUnique(Referencer.ToString());
									}
								}
							}
						SearchedSkeleton.Add(AnimSkeleton);
					}
				}
			}
		}
	}

	ReferenceBP.Sort();
	return ReferenceBP;
}

TArray<FAssetData> GetAnimAssets(FString &ProjectPath, FString &MapPackage, FString &MapName)
{
	FString ShotDir;

	TArray<FString> MapNameSplit;
	MapName.ParseIntoArray(MapNameSplit, TEXT("_"), true);
	if (MapNameSplit.Num() > 2)
		if (MapNameSplit[2].Len() >= 3)
			ShotDir = MapNameSplit[2].Right(3);

	UObjectLibrary *AnimLibrary = UObjectLibrary::CreateLibrary(UAnimSequence::StaticClass(), false, GIsEditor);
	if (AnimLibrary)
	{
		AnimLibrary->AddToRoot();
	}

	// Find AnimSequenc Used By BP Asset
	FString AnimAssetsPath = "/Game/" + ProjectPath + "/SC" + ShotDir + "/Anim";
	AnimLibrary->LoadAssetDataFromPath(*AnimAssetsPath);
	TArray<FAssetData> AnimAssetsData;
	AnimLibrary->GetAssetDataList(AnimAssetsData);

	return AnimAssetsData;
}

TArray<UAnimSequence *> FLoadBP::GetSkeletalMeshAnims(TArray<FAssetData> AnimAssetsData, TArray<TSharedPtr<FString>> LoadedBP)
{
	TArray<UAnimSequence *> SkeletalMeshAnims;

	for (auto AnimAsset : AnimAssetsData)
	{
		FString AnimPath = AnimAsset.ObjectPath.ToString();
		UAnimSequence *AnimSequence = LoadObject<UAnimSequence>(NULL, *AnimPath);
		SkeletalMeshAnims.Add(AnimSequence);

		bool bFindBP = false;
		for (auto BP : LoadedBP)
		{
			UClass *BPClass = FLoadBP::GetBPClass(*BP);
			AActor *BPRootActor = GWorld->SpawnActor<AActor>(BPClass);
			TArray<USkeletalMeshComponent *> BPComponents;
			BPRootActor->GetComponents<USkeletalMeshComponent>(BPComponents);
			if (BPComponents.Num())
			{
				for (auto Component : BPComponents)
				{
					if (!bFindBP)
					{
						if (Component->SkeletalMesh->GetSkeleton() == AnimSequence->GetSkeleton())
						{
							bFindBP = true;
							SkeletalMeshAnims.Remove(AnimSequence);
							break;
						}
					}
				}
			}
			GWorld->RemoveActor(BPRootActor, true);
		}
	}
	return SkeletalMeshAnims;
}

TArray<TArray<UAnimSequence *>> FLoadBP::GetBPAnims(TArray<FAssetData> AnimAssetsData, UClass *BPClass)
{
	TArray<TArray<UAnimSequence *>> BPAnims;

	if (AnimAssetsData.Num())
	{
		AActor *BPRootActor = GWorld->SpawnActor<AActor>(BPClass);
		TArray<USkeletalMeshComponent *> BPComponents;
		BPRootActor->GetComponents<USkeletalMeshComponent>(BPComponents);
		if (BPComponents.Num())
		{
			for (auto Component : BPComponents)
			{
				TArray<UAnimSequence *> AnimSequences;

				for (auto AnimAsset : AnimAssetsData)
				{
					FString AnimPath = AnimAsset.ObjectPath.ToString();
					UAnimSequence *AnimSequence = LoadObject<UAnimSequence>(NULL, *AnimPath);

					if (Component->SkeletalMesh->GetSkeleton() == AnimSequence->GetSkeleton())
						AnimSequences.Add(AnimSequence);
				}
				BPAnims.Add(AnimSequences);
			}
		}
		GWorld->RemoveActor(BPRootActor, true);
	}

	return BPAnims;
}

void FLoadBP::LoadBP(FString &BPPackage, FString &ProjectPath, FString &MapPackage, FString &MapName, FString &LevelSequencePackage, int32 StartFrame)
{
	UClass *BPClass = FLoadBP::GetBPClass(BPPackage);

	if (BPClass != nullptr)
	{
		// UEditorLoadingAndSavingUtils::SaveCurrentLevel();
		UEditorLoadingAndSavingUtils::LoadMap(*MapPackage);
		TArray<FAssetData> AnimAssetsData = GetAnimAssets(ProjectPath, MapPackage, MapName);

		TArray<TArray<UAnimSequence *>> BPAnims = FLoadBP::GetBPAnims(AnimAssetsData, BPClass);

		ULevelSequence *ShotSequence = LoadObject<ULevelSequence>(NULL, *LevelSequencePackage);
		FLoadBP::AddBPActor(BPAnims, ShotSequence, BPClass, StartFrame);

		UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
	}
}

void FLoadBP::AddSkeletalMeshActor(ULevelSequence *ShotSequence, UAnimSequence *AnimSequence, int32 StartFrame)
{
	UMovieScene *MyMovieScene = ShotSequence->GetMovieScene();
	FFrameNumber MySequenceStart = MyMovieScene->GetTickResolution().Numerator * StartFrame / 25;

	// Create SkeletalMesh Actor
	ASkeletalMeshActor *SkeletalMeshActor = Cast<ASkeletalMeshActor>(GEditor->AddActor(GWorld->GetCurrentLevel(), ASkeletalMeshActor::StaticClass(), FTransform(FVector(0))));
	SkeletalMeshActor->SetActorLabel(AnimSequence->GetName());
	SkeletalMeshActor->GetSkeletalMeshComponent()->SetSkeletalMesh(AnimSequence->GetSkeleton()->GetPreviewMesh(true));
	SkeletalMeshActor->GetSkeletalMeshComponent()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	SkeletalMeshActor->GetSkeletalMeshComponent()->AnimationData.AnimToPlay = AnimSequence;

	// Check Actor Bind Exist in Sequence
	int32 BindPosseableCount = MyMovieScene->GetPossessableCount();
	bool bExist = false;
	for (int32 Index = 0; Index < BindPosseableCount; Index++)
	{
		FMovieScenePossessable Possable = MyMovieScene->GetPossessable(Index);
		if (Possable.GetName() == SkeletalMeshActor->GetActorLabel())
		{
			bExist = true;
			break;
		}
	}

	// Add SkeletalMeshActor to LevelSequence
	if (!bExist)
	{
		FGuid SkeltalMeshActorGuid = MyMovieScene->AddPossessable(SkeletalMeshActor->GetActorLabel(), SkeletalMeshActor->GetClass());
		ShotSequence->BindPossessableObject(SkeltalMeshActorGuid, *SkeletalMeshActor, SkeletalMeshActor->GetWorld());

		UMovieSceneSkeletalAnimationTrack *AnimTrack = MyMovieScene->AddTrack<UMovieSceneSkeletalAnimationTrack>(SkeltalMeshActorGuid);
		UAnimSequenceBase *AnimSequenceBase = (UAnimSequenceBase *)AnimSequence;
		AnimTrack->AddNewAnimation(MySequenceStart, AnimSequenceBase);

		UMovieScene3DTransformTrack *SkeletalMeshActorTransTrack = MyMovieScene->AddTrack<UMovieScene3DTransformTrack>(SkeltalMeshActorGuid);
		UMovieScene3DTransformSection *SkelMeshActorTransSection = Cast<UMovieScene3DTransformSection>(SkeletalMeshActorTransTrack->CreateNewSection());
		SkelMeshActorTransSection->SetRange(TRange<FFrameNumber>::All());
		SkeletalMeshActorTransTrack->AddSection(*SkelMeshActorTransSection);
	}
}

void FLoadBP::LoadSkeletonAnim(FString &SkeletonPackage, FString &ProjectPath, FString &MapPackage, FString &MapName, FString &LevelSequencePackage, int32 StartFrame)
{
	USkeleton *Skeleton = LoadObject<USkeleton>(NULL, *SkeletonPackage);

	if (Skeleton)
	{
		// UEditorLoadingAndSavingUtils::SaveCurrentLevel();
		UEditorLoadingAndSavingUtils::LoadMap(*MapPackage);
		TArray<FAssetData> AnimAssetsData = GetAnimAssets(ProjectPath, MapPackage, MapName);
		ULevelSequence *ShotSequence = LoadObject<ULevelSequence>(NULL, *LevelSequencePackage);

		for (auto AnimAsset : AnimAssetsData)
		{
			FString AnimPath = AnimAsset.ObjectPath.ToString();
			UAnimSequence *AnimSequence = LoadObject<UAnimSequence>(NULL, *AnimPath);

			if (Skeleton == AnimSequence->GetSkeleton())
				FLoadBP::AddSkeletalMeshActor(ShotSequence, AnimSequence, StartFrame);
		}
		UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
	}
}

#undef LOCTEXT_NAMESPACE