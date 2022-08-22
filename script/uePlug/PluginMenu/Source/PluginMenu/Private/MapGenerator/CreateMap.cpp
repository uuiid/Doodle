#include "MapGenerator/CreateMap.h"
#include "MapGenerator/ConvertPath.h"
#include "MapGenerator/LoadBP.h"

#include "Engine.h"
#include "Editor.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "HAL/FileManager.h"
#include "Engine/LevelStreamingAlwaysLoaded.h"
#include "LevelSequence/Public/LevelSequence.h"
#include "MovieScene.h"
#include "MovieScenePossessable.h"
#include "MovieScene/Public/MovieSceneSequence.h"
#include "LevelSequence/Public/LevelSequenceActor.h"

#include "AssetRegistryModule.h"
#include "Engine/ObjectLibrary.h"
#include "AssetData.h"

#include "Animation/SkeletalMeshActor.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "MovieSceneTracks/Public/Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "MovieSceneTracks/Public/Tracks/MovieScene3DTransformTrack.h"
#include "MovieSceneTracks/Public/Sections/MovieScene3DTransformSection.h"

#include "ILevelSequenceEditorToolkit.h"
#include "Toolkits/AssetEditorManager.h"

#include "MapGenerator/ImportFbxFileCamera.h"

#define LOCTEXT_NAMESPACE "FCreateMap"

// Create Empty Level and Save
void SaveLevel(FString &SaveMapPath, FString &LevelMap)
{

	// Savel Level
	FString FullPath = FPaths::ProjectContentDir() + SaveMapPath;
	if (!IFileManager::Get().DirectoryExists(*FullPath))
		IFileManager::Get().MakeDirectory(*FullPath);

	FString LevelName = "/Game/" + SaveMapPath + "/" + LevelMap;
	FEditorFileUtils::SaveLevel(GWorld->GetCurrentLevel(), LevelName, &LevelName);
	FEditorFileUtils::SaveCurrentLevel();
}

// Add StreamingLevel
void AddStreamingLevel(FString &LevelStreamingMap)
{
	ULevelStreamingAlwaysLoaded *StreamingLevel = NewObject<ULevelStreamingAlwaysLoaded>(GWorld, NAME_None, RF_Public, nullptr);

	FName SceneMap;
	if (LevelStreamingMap == "Template_Default")
		SceneMap = FName("/Engine/Maps/Templates/Template_Default");
	else
	{
		FString Extension = "umap";
		FString MapFullPath = FConvertPath::ToAbsolutePath(LevelStreamingMap, true, Extension);
		if (IFileManager::Get().FileExists(*MapFullPath))
			SceneMap = FName(*LevelStreamingMap);
		else
			SceneMap = FName("/Engine/Maps/Templates/Template_Default");
	}

	// StreamingLevel Setting
	StreamingLevel->SetWorldAssetByPackageName(SceneMap);
	StreamingLevel->PackageNameToLoad = SceneMap;
	StreamingLevel->bLocked = true;

	GWorld->AddStreamingLevel(StreamingLevel);
	GWorld->FlushLevelStreaming();
}

void AddSkeletalMeshActor(ULevelSequence *ShotSequence, UAnimSequence *AnimSequence, FFrameNumber MySequenceStart)
{
	UMovieScene *MyMovieScene = ShotSequence->GetMovieScene();

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

FString SetupLevelSequence(FString &ProjectPath, FString &Shot, FString &SaveMapPath, FString &LevelMap)
{

	FString LevelSequenceName = "Seq_" + LevelMap;
	FString PackagePath = "/Game/" + SaveMapPath + "/" + LevelSequenceName;
	FString PackgeSavePath = FPaths::ProjectContentDir() + "/" + SaveMapPath + "/" + LevelSequenceName;

	// ULevelSequence * ShotSequence = LoadObject<ULevelSequence>(NULL, *PackagePath);

	UPackage *Package = LoadPackage(NULL, *PackagePath, LOAD_None);
	ULevelSequence *ShotSequence;

	if (Package == nullptr)
	{
		Package = CreatePackage(*PackagePath);
		ShotSequence = NewObject<ULevelSequence>(Package, FName(*LevelSequenceName), RF_Public | RF_Standalone);

		FAssetRegistryModule::AssetCreated(ShotSequence);

		ShotSequence->Initialize();
		Package->MarkPackageDirty();
		UPackage::SavePackage(Package, nullptr, RF_Public | RF_Standalone, *(PackgeSavePath + FPackageName::GetAssetPackageExtension()));
	}
	else
		ShotSequence = LoadObject<ULevelSequence>(NULL, *PackagePath);

	// Load AnimSequecne From Anim Directory in ProjectPath
	UObjectLibrary *AnimLibrary = UObjectLibrary::CreateLibrary(UAnimSequence::StaticClass(), false, GIsEditor);
	if (AnimLibrary)
	{
		AnimLibrary->AddToRoot();
	}

	FString AnimAssetsPath = "/Game/" + ProjectPath + "/" + Shot + "/Anim";
	AnimLibrary->LoadAssetDataFromPath(*AnimAssetsPath);

	TArray<FAssetData> AnimAssetsData;
	AnimLibrary->GetAssetDataList(AnimAssetsData);

	// Get Animaton NumFrames
	int32 SequenceLength = 0;
	if (AnimAssetsData.Num())
	{
		for (auto Anim : AnimAssetsData)
		{
			FString AnimSoftPath = Anim.ObjectPath.ToString();
			UAnimSequence *AnimSequenceTemp = LoadObject<UAnimSequence>(NULL, *AnimSoftPath);
			// int32 Max = AnimSequenceTemp->NumFrames;
			int32 Max = AnimSequenceTemp->GetNumberOfFrames();
			if (Max > SequenceLength)
				SequenceLength = AnimSequenceTemp->GetNumberOfFrames();
		}
	}
	else
		SequenceLength = 200;

	// LevelSequence Setup
	UMovieScene *MyMovieScene = ShotSequence->GetMovieScene();
	int32 MySeqStart = 71;
	int32 MySeqEnd = MySeqStart + SequenceLength - 1;

	MyMovieScene->SetDisplayRate(FFrameRate(25, 1));
	MyMovieScene->SetWorkingRange(static_cast<float>(MySeqStart) / 25, static_cast<float>(MySeqEnd + 10) / 25);
	MyMovieScene->SetViewRange(static_cast<float>(MySeqStart) / 25, static_cast<float>(MySeqEnd + 10) / 25);

	FFrameRate MyTickResolution = MyMovieScene->GetTickResolution();

	FFrameNumber MySequenceStart = MyTickResolution.Numerator * MySeqStart / 25;
	FFrameNumber MySequenceEnd = MyTickResolution.Numerator * MySeqEnd / 25;
	TRange<FFrameNumber> MySectionRange = TRange<FFrameNumber>(MySequenceStart, MySequenceEnd);

	// MyMovieScene->SetSelectionRange(MySectionRange);
	MyMovieScene->SetPlaybackRangeLocked(false);
	MyMovieScene->SetPlaybackRange(MySequenceStart, (MySequenceEnd - MySequenceStart).Value, true);

	// Add SkeletalMesh Actor to LevelSequence
	for (auto AnimAsset : AnimAssetsData)
	{
		FString AnimPath = AnimAsset.ObjectPath.ToString();
		UAnimSequence *AnimSequence = LoadObject<UAnimSequence>(NULL, *AnimPath);

		if (AnimSequence)
		{
			AddSkeletalMeshActor(ShotSequence, AnimSequence, MySequenceStart);
		}
	}

	// Save DirtyPackage
	UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);

	return PackagePath;
}

// Create Empty Level Sequence
FString SetupBPLevelSequence(FString &ProjectPath, FString &Shot, FString &SaveMapPath, FString &LevelMap, TArray<TSharedPtr<FString>> LoadedBP)
{

	FString LevelSequenceName = "Seq_" + LevelMap;
	FString PackagePath = "/Game/" + SaveMapPath + "/" + LevelSequenceName;
	FString PackgeSavePath = FPaths::ProjectContentDir() + "/" + SaveMapPath + "/" + LevelSequenceName;

	// ULevelSequence * ShotSequence = LoadObject<ULevelSequence>(NULL, *PackagePath);

	UPackage *Package = LoadPackage(NULL, *PackagePath, LOAD_None);
	;
	ULevelSequence *ShotSequence;

	if (Package == nullptr)
	{
		Package = CreatePackage(*PackagePath);
		ShotSequence = NewObject<ULevelSequence>(Package, FName(*LevelSequenceName), RF_Public | RF_Standalone);

		FAssetRegistryModule::AssetCreated(ShotSequence);

		ShotSequence->Initialize();
		Package->MarkPackageDirty();
		UPackage::SavePackage(Package, nullptr, RF_Public | RF_Standalone, *(PackgeSavePath + FPackageName::GetAssetPackageExtension()));
	}
	else
		ShotSequence = LoadObject<ULevelSequence>(NULL, *PackagePath);

	// Load AnimSequecne From Anim Directory in ProjectPath
	UObjectLibrary *AnimLibrary = UObjectLibrary::CreateLibrary(UAnimSequence::StaticClass(), false, GIsEditor);
	if (AnimLibrary)
	{
		AnimLibrary->AddToRoot();
	}

	FString AnimAssetsPath = "/Game/" + ProjectPath + "/" + Shot + "/Anim";
	AnimLibrary->LoadAssetDataFromPath(*AnimAssetsPath);

	TArray<FAssetData> AnimAssetsData;
	AnimLibrary->GetAssetDataList(AnimAssetsData);

	// Get Animaton NumFrames
	int32 SequenceLength = 0;
	if (AnimAssetsData.Num())
	{
		for (auto Anim : AnimAssetsData)
		{
			FString AnimSoftPath = Anim.ObjectPath.ToString();
			UAnimSequence *AnimSequenceTemp = LoadObject<UAnimSequence>(NULL, *AnimSoftPath);
			int32 Max = AnimSequenceTemp->GetNumberOfFrames();
			if (Max > SequenceLength)
				SequenceLength = AnimSequenceTemp->GetNumberOfFrames();
		}
	}
	else
		SequenceLength = 200;

	// LevelSequence Setup
	UMovieScene *MyMovieScene = ShotSequence->GetMovieScene();
	int32 MySeqStart = 71;
	int32 MySeqEnd = MySeqStart + SequenceLength - 1;

	MyMovieScene->SetDisplayRate(FFrameRate(25, 1));
	MyMovieScene->SetWorkingRange(static_cast<float>(MySeqStart) / 25, static_cast<float>(MySeqEnd + 10) / 25);
	MyMovieScene->SetViewRange(static_cast<float>(MySeqStart) / 25, static_cast<float>(MySeqEnd + 10) / 25);

	FFrameRate MyTickResolution = MyMovieScene->GetTickResolution();

	FFrameNumber MySequenceStart = MyTickResolution.Numerator * MySeqStart / 25;
	FFrameNumber MySequenceEnd = MyTickResolution.Numerator * MySeqEnd / 25;
	TRange<FFrameNumber> MySectionRange = TRange<FFrameNumber>(MySequenceStart, MySequenceEnd);

	// MyMovieScene->SetSelectionRange(MySectionRange);
	MyMovieScene->SetPlaybackRangeLocked(false);
	MyMovieScene->SetPlaybackRange(MySequenceStart, (MySequenceEnd - MySequenceStart).Value, true);

	if (AnimAssetsData.Num())
	{
		// Add Skeletal Mesh Actors
		TArray<UAnimSequence *> SkeletalMeshAnims = FLoadBP::GetSkeletalMeshAnims(AnimAssetsData, LoadedBP);
		for (auto AnimSequence : SkeletalMeshAnims)
		{
			AddSkeletalMeshActor(ShotSequence, AnimSequence, MySequenceStart);
		}

		// Add BP Actors
		for (auto BP : LoadedBP)
		{
			UClass *BPClass = FLoadBP::GetBPClass(*BP);
			TArray<TArray<UAnimSequence *>> BPAnims = FLoadBP::GetBPAnims(AnimAssetsData, BPClass);
			FLoadBP::AddBPActor(BPAnims, ShotSequence, BPClass);
		}
	}
	// Save DirtyPackage
	UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);

	return PackagePath;
}

FString SetupBPLevelSequence(FString &ProjectPath, FString &Shot, FString &SaveMapPath, FString &LevelMap, TArray<TSharedPtr<struct FBPInfo>> AllBPInfo)
{

	FString LevelSequenceName = "Seq_" + LevelMap;
	FString PackagePath = "/Game/" + SaveMapPath + "/" + LevelSequenceName;
	FString PackgeSavePath = FPaths::ProjectContentDir() + "/" + SaveMapPath + "/" + LevelSequenceName;

	// ULevelSequence * ShotSequence = LoadObject<ULevelSequence>(NULL, *PackagePath);

	UPackage *Package = LoadPackage(NULL, *PackagePath, LOAD_None);
	;
	ULevelSequence *ShotSequence;

	if (Package == nullptr)
	{
		Package = CreatePackage(*PackagePath);
		ShotSequence = NewObject<ULevelSequence>(Package, FName(*LevelSequenceName), RF_Public | RF_Standalone);

		FAssetRegistryModule::AssetCreated(ShotSequence);

		ShotSequence->Initialize();
		Package->MarkPackageDirty();
		UPackage::SavePackage(Package, nullptr, RF_Public | RF_Standalone, *(PackgeSavePath + FPackageName::GetAssetPackageExtension()));
	}
	else
		ShotSequence = LoadObject<ULevelSequence>(NULL, *PackagePath);

	// Load AnimSequecne From Anim Directory in ProjectPath
	UObjectLibrary *AnimLibrary = UObjectLibrary::CreateLibrary(UAnimSequence::StaticClass(), false, GIsEditor);
	if (AnimLibrary)
	{
		AnimLibrary->AddToRoot();
	}

	FString AnimAssetsPath = "/Game/" + ProjectPath + "/" + Shot + "/Anim";
	AnimLibrary->LoadAssetDataFromPath(*AnimAssetsPath);

	TArray<FAssetData> AnimAssetsData;
	AnimLibrary->GetAssetDataList(AnimAssetsData);

	// Get Animaton NumFrames
	int32 SequenceLength = 0;
	if (AnimAssetsData.Num())
	{
		for (auto Anim : AnimAssetsData)
		{
			FString AnimSoftPath = Anim.ObjectPath.ToString();
			UAnimSequence *AnimSequenceTemp = LoadObject<UAnimSequence>(NULL, *AnimSoftPath);
			int32 Max = AnimSequenceTemp->GetNumberOfFrames();
			if (Max > SequenceLength)
				SequenceLength = AnimSequenceTemp->GetNumberOfFrames();
		}
	}
	else
		SequenceLength = 200;

	// LevelSequence Setup
	UMovieScene *MyMovieScene = ShotSequence->GetMovieScene();
	int32 MySeqStart = 71;
	int32 MySeqEnd = MySeqStart + SequenceLength - 1;

	MyMovieScene->SetDisplayRate(FFrameRate(25, 1));
	MyMovieScene->SetWorkingRange(static_cast<float>(MySeqStart) / 25, static_cast<float>(MySeqEnd + 10) / 25);
	MyMovieScene->SetViewRange(static_cast<float>(MySeqStart) / 25, static_cast<float>(MySeqEnd + 10) / 25);

	FFrameRate MyTickResolution = MyMovieScene->GetTickResolution();

	FFrameNumber MySequenceStart = MyTickResolution.Numerator * MySeqStart / 25;
	FFrameNumber MySequenceEnd = MyTickResolution.Numerator * MySeqEnd / 25;
	TRange<FFrameNumber> MySectionRange = TRange<FFrameNumber>(MySequenceStart, MySequenceEnd);

	// MyMovieScene->SetSelectionRange(MySectionRange);
	MyMovieScene->SetPlaybackRangeLocked(false);
	MyMovieScene->SetPlaybackRange(MySequenceStart, (MySequenceEnd - MySequenceStart).Value, true);

	if (AnimAssetsData.Num())
	{
		TArray<TSharedPtr<FString>> LoadedBP;
		// Add BP Actors
		for (auto BPInfo : AllBPInfo)
			if (BPInfo->bLoaded)
			{
				LoadedBP.Add(MakeShareable(new FString(BPInfo->BPPackage)));
				UClass *BPClass = FLoadBP::GetBPClass(BPInfo->BPPackage);
				TArray<TArray<UAnimSequence *>> BPAnims = FLoadBP::GetBPAnims(AnimAssetsData, BPClass);
				FLoadBP::AddBPActor(BPAnims, ShotSequence, BPClass, BPInfo->StartFrame);
			}

		// Add Skeletal Mesh Actors

		TArray<UAnimSequence *> SkeletalMeshAnims = FLoadBP::GetSkeletalMeshAnims(AnimAssetsData, LoadedBP);
		for (auto AnimSequence : SkeletalMeshAnims)
		{
			AddSkeletalMeshActor(ShotSequence, AnimSequence, MySequenceStart);
		}
	}
	// Save DirtyPackage
	UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);

	return PackagePath;
}

void ImportCamera(FString &LevelSequencePath, FString &FbxCameraDir, FString &Shot)
{
	ULevelSequence *ShotSequence = LoadObject<ULevelSequence>(NULL, *LevelSequencePath);
	if (ShotSequence != nullptr)
	{
		FSoftObjectPath LevelSequenceSoftPath = FSoftObjectPath(LevelSequencePath);
		UObject *LoadedObject = LevelSequenceSoftPath.TryLoad();
		UAssetEditorSubsystem *AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (LoadedObject != nullptr)
		{
			AssetEditorSubsystem->OpenEditorForAsset(LoadedObject);
		}
		IAssetEditorInstance *AssetEditor = AssetEditorSubsystem->FindEditorForAsset(ShotSequence, true);
		ILevelSequenceEditorToolkit *LevelSequenceEditor = CastChecked<ILevelSequenceEditorToolkit>(AssetEditor);
		ISequencer *ShotSequencer = LevelSequenceEditor->GetSequencer().Get();

		TMap<FGuid, FString> ObjectBindingMap;

		FString ShotCameraDir = FbxCameraDir + "/" + Shot;

		TArray<FString> FoundFiles;
		if (IFileManager::Get().DirectoryExists(*ShotCameraDir))
		{
			IFileManager::Get().FindFiles(FoundFiles, *ShotCameraDir, TEXT(".fbx"));
			if (FoundFiles.Num())
			{
				if (FoundFiles[0].StartsWith(TEXT("CAM"), ESearchCase::IgnoreCase))
				{
					FString FbxCameraPath = ShotCameraDir + "/" + FoundFiles[0];
					FImportFbxFileCamera::ImportCameraFromFbxFile(FbxCameraPath, ShotSequence->MovieScene, ShotSequencer, ObjectBindingMap);
				}
			}
		}
	}
}

void FCreateMap::CreateMap(FString &ProjectPath, FString &FbxCameraDir, FString &Shot, FString &LevelMap, FString &LevelStreamingMap, bool IsSaveInMap)
{

	GEditor->CreateNewMapForEditing();

	FString SaveMapPath;
	if (IsSaveInMap)
		SaveMapPath = ProjectPath + "/" + "map";
	else
		SaveMapPath = ProjectPath + "/" + Shot;

	SaveLevel(SaveMapPath, LevelMap);

	AddStreamingLevel(LevelStreamingMap);

	FString LevelSequecnePath = SetupLevelSequence(ProjectPath, Shot, SaveMapPath, LevelMap);

	ImportCamera(LevelSequecnePath, FbxCameraDir, Shot);

	SaveLevel(SaveMapPath, LevelMap);
}

void FCreateMap::CreateBPMap(FString &ProjectPath, FString &FbxCameraDir, FString &Shot, FString &LevelMap, FString &LevelStreamingMap, bool IsSaveInMap, TArray<TSharedPtr<FString>> LoadedBP)
{

	GEditor->CreateNewMapForEditing();

	FString SaveMapPath;
	if (IsSaveInMap)
		SaveMapPath = ProjectPath + "/" + "map";
	else
		SaveMapPath = ProjectPath + "/" + Shot;

	SaveLevel(SaveMapPath, LevelMap);

	AddStreamingLevel(LevelStreamingMap);

	FString LevelSequecnePackage = SetupBPLevelSequence(ProjectPath, Shot, SaveMapPath, LevelMap, LoadedBP);

	ImportCamera(LevelSequecnePackage, FbxCameraDir, Shot);

	SaveLevel(SaveMapPath, LevelMap);
}

void FCreateMap::CreateBPMap(FString &ProjectPath, FString &FbxCameraDir, FString &Shot, FString &LevelMap, FString &LevelStreamingMap, bool IsSaveInMap, TArray<TSharedPtr<struct FBPInfo>> AllBPInfo)
{

	GEditor->CreateNewMapForEditing();

	FString SaveMapPath;
	if (IsSaveInMap)
		SaveMapPath = ProjectPath + "/" + "map";
	else
		SaveMapPath = ProjectPath + "/" + Shot;

	SaveLevel(SaveMapPath, LevelMap);

	AddStreamingLevel(LevelStreamingMap);

	FString LevelSequecnePackage = SetupBPLevelSequence(ProjectPath, Shot, SaveMapPath, LevelMap, AllBPInfo);

	ImportCamera(LevelSequecnePackage, FbxCameraDir, Shot);

	SaveLevel(SaveMapPath, LevelMap);
}

#undef LOCTEXT_NAMESPACE

// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *OpenDirectory);