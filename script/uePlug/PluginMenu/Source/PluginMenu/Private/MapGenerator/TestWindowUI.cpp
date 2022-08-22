#include "MapGenerator/TestWindowUI.h"

#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SButton.h"
#include "Input/Reply.h"

#include "AssetToolsModule.h"
#include "AssetTools/Public/IAssetTools.h"
#include "GameFramework/Actor.h"
#include "Engine/Texture2D.h"
#include "Engine.h"
#include "Editor.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
// #include "Private/ContentBrowserSingleton.h"
#include "AssetRegistryModule.h"
#include "AssetData.h"
#include "Misc/Paths.h"
#include "Misc/AssetRegistryInterface.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
// #include "Private/SAssetView.h"
#include "EditorDirectories.h"
#include "LevelSequence/Public/LevelSequence.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Animation/SkeletalMeshActor.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "LevelSequence/Public/LevelSequence.h"
#include "MovieScene.h"
#include "MovieSceneTracks/Public/Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Engine/Public/TextureResource.h"
#include "Toolkits/AssetEditorManager.h"
#include "ILevelSequenceEditorToolkit.h"
#include "ISequencer.h"
#include "AssetToolsModule.h"
#include "AssetTools/Public/IAssetTools.h"

#include "Engine/Texture2D.h"

#include "Editor/UnrealEd/Public/FileHelpers.h"
// #include "Private/ContentBrowserUtils.h"

#include "MapGenerator/ConvertPath.h"
#include "MapGenerator/CreateMapUtils.h"

#include "Subsystems/AssetEditorSubsystem.h"
#include "ILevelSequenceEditorToolkit.h"

//#include "Public/AssetManagerEditorModule.h"

#define LOCTEXT_NAMESPACE "STestWindowUI"

void STestWindowUI::Construct(const FArguments &InArgs)
{
	ChildSlot
		[SNew(SCanvas) + SCanvas::Slot().Position(FVector2D(320, 60)).Size(FVector2D(50, 50))[SNew(SButton).Text(FText::FromString(TEXT("Check"))).OnClicked(this, &STestWindowUI::CheckReference)] + SCanvas::Slot().Position(FVector2D(100, 60)).Size(FVector2D(200, 50))[SAssignNew(TextFolder, SEditableTextBox).Text(FText::FromString(TEXT("")))] + SCanvas::Slot().Position(FVector2D(100, 150)).Size(FVector2D(50, 50))[SNew(SButton).Text(FText::FromString(TEXT("Remove"))).OnClicked(this, &STestWindowUI::RemoveActor)]

	];
}

FReply STestWindowUI::CheckReference()
{
	/*ULevel * CurrentLevel = GWorld->GetCurrentLevel();
	TArray<AActor*> AllActors = CurrentLevel->Actors;
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::FromInt(AllActors.Num()));*/

	// FString InMapPackage = "/Game/YSJ_VFX_project/EP112/SC036/YSJ_112_036_VFX";
	// UWorld * World = UEditorLoadingAndSavingUtils::LoadMap(*InMapPackage);

	// FString Folder = "VFX";
	// TArray<AActor*> VFXActors = FCreateMapUtils::GetActorsInFolder(Folder);

	////Select Actors In Folder
	// GEditor->SelectNone(false, false, false);
	// for (auto Actor : VFXActors)
	//{
	//	GEditor->SelectActor(Actor, true, false, false, false);
	// }

	////Copy And Paste Actors
	// GEditor->CopySelectedActorsToClipboard(GWorld, false, false, false);

	// FEditorFileUtils::SaveCurrentLevel();

	// GEditor->CreateNewMapForEditing();
	// GEditor->PasteSelectedActorsFromClipboard(GWorld, FText::FromString(""), EPasteTo::PT_OriginalLocation);

	// FString OutputPath = "YSJ_VFX_project/EP112/map";
	// FString OutMapName = "YSJ_112_036_VFX";
	// FCreateMapUtils::SaveLevel(OutputPath, OutMapName);

	// FString LevelSequencePackage = "/Game/YSJ_VFX_project/EP112/SC036/Seq_YSJ_112_036_VFX";
	//
	//

	// ULevelSequence * Sequence = LoadObject<ULevelSequence>(NULL, *LevelSequencePackage);

	// FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(FName("AssetTools"));
	// IAssetTools& AssetTool = AssetToolsModule.Get();

	// FString DuplicateSequencePackage = "/Game/YSJ_VFX_project/EP112/map/Seq_YSJ_112_036_VFX";
	// ULevelSequence * DuplicateSequence = LoadObject<ULevelSequence>(NULL, *DuplicateSequencePackage);

	// if (DuplicateSequence == nullptr)
	//	DuplicateSequence = (ULevelSequence*)AssetTool.DuplicateAsset(TEXT("Seq_YSJ_112_036_VFX"), TEXT("/Game/YSJ_VFX_project/EP112/map"), Sequence);

	// FCreateMapUtils::FixActorBinds(DuplicateSequence);

	///*FString Folder = "VFX";
	// TArray<AActor*> VFXActors = FCreateMapUtils::GetActorsInFolder(Folder);

	// for (auto Actor : VFXActors)
	//{
	//	TSet<UActorComponent*> ActorComponents = Actor->GetComponents();
	//	for (auto it = ActorComponents.CreateIterator(); it; ++it)
	//	{
	//		UActorComponent* Component = *it;
	//		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *Component->GetName());
	//	}
	// }*/

	// UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);

	/*UObject * Object = LoadObject<UObject>(NULL, TEXT("/Game/wenli_107"));

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *Object->GetClass()->GetName());

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(FName("AssetTools"));
	IAssetTools& AssetTool = AssetToolsModule.Get();

	FAssetRenameData RenameData = FAssetRenameData(Object, TEXT("/Game/aaa"), TEXT("wenli_107"));
	TArray<FAssetRenameData> RenameAssets;
	RenameAssets.Add(RenameData);

	AssetTool.RenameAssets(RenameAssets);*/

	// UObjectLibrary * AnimLibrary = UObjectLibrary::CreateLibrary(UTexture2D::StaticClass(), false, GIsEditor);
	// if (AnimLibrary)
	//{
	//	AnimLibrary->AddToRoot();
	// }

	// FString AnimAssetsPath = "/Game/" ;
	// AnimLibrary->LoadAssetDataFromPath(*AnimAssetsPath);

	// TArray<FAssetData> AssetsData;
	// AnimLibrary->GetAssetDataList(AssetsData);

	////Get Animaton NumFrames
	// if (AssetsData.Num())
	//{
	//	for (auto Tex : AssetsData)
	//	{
	//		FString TexSoftPath = Tex.ObjectPath.ToString();
	//		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *TexSoftPath);
	//
	//	}
	// }
	//
	/*FString Folder = "G:/CPP3/Content/";
	TArray<FString> FoundFiles;
	IFileManager::Get().FindFiles(FoundFiles, *Folder, TEXT(".uasset"));

	if (FoundFiles.Num())
		for (auto File : FoundFiles)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *File);
		}*/

	// UObjectLibrary * AnimLibrary = UObjectLibrary::CreateLibrary(UObjectRedirector::StaticClass(), false, GIsEditor);
	// if (AnimLibrary)
	//{
	//	AnimLibrary->AddToRoot();
	// }

	// FString Folder = TextFolder->GetText().ToString();
	// FString AnimAssetsPath = "/Game/" + Folder;
	// AnimLibrary->LoadAssetDataFromPath(*AnimAssetsPath);

	// TArray<FAssetData> AssetsData;
	// AnimLibrary->GetAssetDataList(AssetsData);

	// ObjectRedirectors.Reset();
	// if (AssetsData.Num())
	//{
	//	for (auto Asset : AssetsData)
	//	{

	//		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *Asset.ToSoftObjectPath().ToString());
	//		//UObjectRedirector* Redirector = (UObjectRedirector*)Asset.ToSoftObjectPath().TryLoad();
	//		UObjectRedirector* Redirector = LoadObject<UObjectRedirector>(NULL, *Asset.ToSoftObjectPath().ToString());
	//		ObjectRedirectors.Add(Redirector);

	//	}

	//}
	FString LevelSequencePackage = TextFolder->GetText().ToString();
	ULevelSequence *Sequence = LoadObject<ULevelSequence>(NULL, *LevelSequencePackage);
	UAssetEditorSubsystem *AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

	if (Sequence != nullptr)
	{
		AssetEditorSubsystem->OpenEditorForAsset(Sequence);
	}

	IAssetEditorInstance *AssetEditor = AssetEditorSubsystem->FindEditorForAsset(Sequence, true);
	ILevelSequenceEditorToolkit *LevelSequenceEditor = (ILevelSequenceEditorToolkit*)AssetEditor;

	ISequencer *ShotSequencer = LevelSequenceEditor->GetSequencer().Get();

	TArrayView<TWeakObjectPtr<UObject>> BindObjects;

	UMovieScene *MyMovieScene = Sequence->MovieScene;
	int32 PosseableCount = MyMovieScene->GetPossessableCount();

	for (int i = 0; i < PosseableCount; i++)
	{
		FMovieScenePossessable Possable = MyMovieScene->GetPossessable(i);
		BindObjects = ShotSequencer->FindObjectsInCurrentSequence(Possable.GetGuid());

		if (BindObjects.Num())
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Possable.GetName());
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *FString::FromInt(BindObjects.Num()));

			for (auto Object : BindObjects)
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, *Object->GetFName().ToString());
		}
	}
	/*for (int i = 0; i < PosseableCount; i++)
	{

		FMovieScenePossessable Possable = MyMovieScene->GetPossessable(i);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Possable.GetName());

	}*/

	FString Folder = "VFX";
	TArray<AActor *> VFXActors = FCreateMapUtils::GetActorsInFolder(Folder);
	for (auto Actor : VFXActors)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, *Actor->GetFName().ToString());
	}
	return FReply::Handled();
}

void MoveAssetInFolder(FString Folder, FString OutFolder, bool bMoveFolder)
{

	TArray<FString> FoundFiles;
	IFileManager::Get().FindFiles(FoundFiles, *Folder, TEXT(".uasset"));

	TArray<FAssetRenameData> RenameAssets;
	if (FoundFiles.Num())
		for (auto File : FoundFiles)
		{

			FString AbsoluteFilePath = Folder + "/" + File;
			FString Package = FConvertPath::ToRelativePath(AbsoluteFilePath, false);

			UObject *Object = LoadObject<UObject>(NULL, *Package);
			FString OutPath = "/Game/" + OutFolder + "/";
			FString OutPackage = OutPath + Package.RightChop(6);
			FString OutPackageName = FConvertPath::GetPackageName(OutPackage);
			FString OutPackagePath = FConvertPath::GetPackagePath(OutPackage);

			// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, *Package);
			// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Blue, *OutPackagePath);

			FAssetRenameData RenameData = FAssetRenameData(Object, *OutPackagePath, *OutPackageName);
			RenameAssets.Add(RenameData);
		}

	if (RenameAssets.Num())
	{
		FAssetToolsModule &AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(FName("AssetTools"));
		IAssetTools &AssetTool = AssetToolsModule.Get();
		AssetTool.RenameAssets(RenameAssets);
	}
}

TArray<FString> STestWindowUI::FindSubDirs(FString Folder)
{
	TArray<FString> SubDirs;
	FString FolderPath = Folder + "/*";
	IFileManager::Get().FindFiles(SubDirs, *FolderPath, false, true);
	if (SubDirs.Num())
		for (auto Dir : SubDirs)
		{
			FoundDirs.Add(Folder + "/" + Dir);
			// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, *(Folder + "/" + Dir));
			FindSubDirs(Folder + "/" + Dir);
		}
	return FoundDirs;
}

int32 AutoRename1(FString InFileName, int32 i, FString Ext)
{
	FString File = InFileName + "_" + FString::FromInt(i) + "." + Ext;

	if (!IFileManager::Get().FileExists(*File))
		return i;
	else
		return AutoRename1(InFileName, i + 1, Ext);
}

FReply STestWindowUI::RemoveActor()
{
	FAssetToolsModule &AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(FName("AssetTools"));
	IAssetTools &AssetTool = AssetToolsModule.Get();

	/*FString Package = "/Game/YSJ_VFX_project/EP063/sc042/mat/P_circle_fire/mf_GRIMColorCollection";
	UObject * Object = LoadObject<UObject>(NULL, *Package);

	FString Package1 = "/Game/YSJ_VFX_project/EP063/sc042/mat/P_circle_fire/T_Rift_Orb_flibbook";
	UObject * Object1 = LoadObject<UObject>(NULL, *Package1);

	FAssetRenameData RenameData = FAssetRenameData(Object, TEXT("/Game"), TEXT("mf_GRIMColorCollection"));
	FAssetRenameData RenameData1 = FAssetRenameData(Object1, TEXT("/Game"), TEXT("T_Rift_Orb_flibbook"));


	RenameAssets.Add(RenameData);
	RenameAssets.Add(RenameData1);

	AssetTool.RenameAssets(RenameAssets);*/

	/*UObjectLibrary * AnimLibrary = UObjectLibrary::CreateLibrary(UObject::StaticClass(), false, GIsEditor);
	if (AnimLibrary)
	{
		AnimLibrary->AddToRoot();
	}

	FString SourceFolder = TextFolder->GetText().ToString();
	FString AnimAssetsPath = "/Game/" + SourceFolder;
	AnimLibrary->LoadAssetDataFromPath(*AnimAssetsPath);

	TArray<FAssetData> AssetsData;
	AnimLibrary->GetAssetDataList(AssetsData);

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *FString::FromInt(AssetsData.Num()));
	for (auto Asset : AssetsData)
	{
		UObject* Object = LoadObject<UObject>(NULL, *Asset.ToSoftObjectPath().ToString());
		FString Class = Object->GetClass()->GetName();
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *Class);
	}*/

	FString Path = TextFolder->GetText().ToString();
	ULevelSequence *MySeq = LoadObject<ULevelSequence>(NULL, *Path);
	UMovieScene *MyMovieScene = MySeq->MovieScene;
	int32 PosseableCount = MyMovieScene->GetPossessableCount();

	FString VFXFolder = "VFX";
	TArray<AActor *> VFXActors = FCreateMapUtils::GetActorsInFolder(VFXFolder);
	for (auto Actor : VFXActors)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Actor->GetActorLabel());
	}
	for (int i = 0; i < PosseableCount; i++)
	{
		FMovieScenePossessable Possable = MyMovieScene->GetPossessable(i);
		bool bFind = false;
		for (auto Actor : VFXActors)
		{
			if (Actor->GetActorLabel() == Possable.GetName())
			{
				bFind = true;
				break;
			}
		}
		if (Possable.GetParent().IsValid())
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Possable.GetName());
		if (!bFind)
		{
			bool bDelete = true;
			for (auto Actor : VFXActors)
			{
				if (Actor->GetActorLabel().Contains(Possable.GetName().Left(6), ESearchCase::IgnoreCase, ESearchDir::FromStart))
				{
					FString Class = Actor->GetClass()->GetName();
					// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, *Class);
					if (Class == "Emitter")
					{
						bDelete = false;
						break;
					}
				}
			}
			if (!bDelete)
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, Possable.GetName());
		}
	}

	/*for (auto Asset : AssetsData)
	{
		UObject* Object = LoadObject<UObject>(NULL, *Asset.ToSoftObjectPath().ToString());
		FString Package = Asset.PackageName.ToString();
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *Package);
		FString Class = Object->GetClass()->GetName();

		FString OutPath, OutPackage;
		FString OutFolder = "Out";
		if (Class == "Texture2D")
			OutPath = "/Game/" + OutFolder + "/" + "Textures/";
		else if (Class == "Material" || Class == "MaterialInstanceConstant" || Class == "MaterialFunction")
			OutPath = "/Game/" + OutFolder + "/" + "Materials/";
		else if (Class == "StaticMesh")
			OutPath = "/Game/" + OutFolder + "/" + "Meshes/";
		else if (Class == "ParticleSystem")
			OutPath = "/Game/" + OutFolder + "/" + "Particles/";
		else
			OutPath = "/Game/" + OutFolder + "/" + "Others/";

		OutPackage = OutPath + FConvertPath::GetPackageName(Package);


		FString Ext = "uasset";
		FString OutFile = FConvertPath::ToAbsolutePath(OutPackage, true, Ext);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *OutFile);
		if (IFileManager::Get().FileExists(*OutFile))
		{
			int32 FileVersion = AutoRename1(FConvertPath::ToAbsolutePath(OutPackage), 0, Ext);
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *FString::FromInt(FileVersion));
			OutPackage = OutPath + FConvertPath::GetPackageName(Package) + "_" + FString::FromInt(FileVersion);
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *OutPackage);
		}
		FString OutPackageName = FConvertPath::GetPackageName(OutPackage);
		FString OutPackagePath = FConvertPath::GetPackagePath(OutPackage);

		FAssetRenameData RenameData = FAssetRenameData(Object, *OutPackagePath, *OutPackageName);
		TArray<FAssetRenameData> RenameAssets;
		RenameAssets.Add(RenameData);
		AssetTool.RenameAssets(RenameAssets);
	}*/

	/*for (auto Asset : AssetsData)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *Asset.AssetName.ToString());
		UObject* Object = LoadObject<UObject>(NULL, *Asset.ToSoftObjectPath().ToString());
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *Object->GetClass()->GetName());

	}*/

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *OpenDirectory);