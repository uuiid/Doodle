#include "MapGenerator/CreateMapUtils.h"

#include "Engine.h"
#include "Editor.h"
#include "LevelSequence/Public/LevelSequence.h"

#include "FileManager.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "AssetTools/Public/IAssetTools.h"
#include "AssetEditorManager.h"
#include "ContentBrowser/Private/ContentBrowserUtils.h"
#include "Private/LevelSequenceEditorToolkit.h"

#include "MapGenerator/ConvertPath.h"
#include "MapGenerator/ImportFbxFileCamera.h"

#define LOCTEXT_NAMESPACE "FCreateMapUtils"

void FCreateMapUtils::SaveLevel(FString& LevelPackagePath, FString& LevelName)
{

	//Savel Level
	FString AbsolutePath = FPaths::ProjectContentDir() + LevelPackagePath;
	if (!IFileManager::Get().DirectoryExists(*AbsolutePath))
		IFileManager::Get().MakeDirectory(*AbsolutePath);

	FString LevelPackage = "/Game/" + LevelPackagePath + "/" + LevelName;
	FEditorFileUtils::SaveLevel(GWorld->GetCurrentLevel(), LevelPackage, &LevelPackage);
	FEditorFileUtils::SaveCurrentLevel();
}

void FCreateMapUtils::AddStreamingLevel(FString& StreamingLevelMapPackage)
{
	ULevelStreamingAlwaysLoaded * StreamingLevel = NewObject<ULevelStreamingAlwaysLoaded>(GWorld, NAME_None, RF_Public, nullptr);

	FName SceneMapPackage;
	if (StreamingLevelMapPackage == "Template_Default")
		SceneMapPackage = FName("/Engine/Maps/Templates/Template_Default");
	else
	{
		FString Extension = "umap";
		FString AbsoluteMapPath = FConvertPath::ToAbsolutePath(StreamingLevelMapPackage, true, Extension);

		if (IFileManager::Get().FileExists(*AbsoluteMapPath))
			SceneMapPackage = FName(*StreamingLevelMapPackage);
		else
			SceneMapPackage = FName("/Engine/Maps/Templates/Template_Default");
	}


	//StreamingLevel Setting
	StreamingLevel->SetWorldAssetByPackageName(SceneMapPackage);
	StreamingLevel->PackageNameToLoad = SceneMapPackage;
	StreamingLevel->bLocked = true;

	GWorld->AddStreamingLevel(StreamingLevel);
	GWorld->FlushLevelStreaming();
}

void FCreateMapUtils::RemoveStreamingLevel(FString& StreamingLevelMapPackage)
{
	
}


TArray<FString> GetSequenceReferencers(FString& MapPackage)
{
	TArray<FString> SequenceReferencers;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FName> SoftdReferencers;
	AssetRegistryModule.Get().GetReferencers(FName(*MapPackage), SoftdReferencers, EAssetRegistryDependencyType::Soft);

	if (SoftdReferencers.Num())
	{
		for (auto Referencer : SoftdReferencers)
		{
			ULevelSequence * Sequence = LoadObject<ULevelSequence>(NULL, *Referencer.ToString());
			if (Sequence != nullptr)
				SequenceReferencers.Add(Referencer.ToString());
		}

	}
	return SequenceReferencers;

}

bool CheckFolderPath(AActor* InActor,FString DestFolder)
{
	FString ActorFolderPath = InActor->GetFolderPath().ToString();

	TArray<FString> SplitPath;
	ActorFolderPath.ParseIntoArray(SplitPath, TEXT("/"), ESearchCase::IgnoreCase);
	FString ActorTopFolder = SplitPath[0];

	if (ActorTopFolder.ToUpper() == DestFolder.ToUpper())
		return true;
	else if (ActorTopFolder == "None")
	{
		AActor * AttachParentActor = InActor->GetAttachParentActor();
		if (AttachParentActor)
			return CheckFolderPath(AttachParentActor, DestFolder);
		else
			return false;
	}
	else
		return false;

	
}


TArray<AActor*> FCreateMapUtils::GetActorsInFolder(FString& FolderInWorld)
{
	TArray<AActor*> AllActors, FolderActors;

	ULevel * CurrentLevel = GWorld->GetCurrentLevel();
	AllActors = CurrentLevel->Actors;

	for (auto Actor : AllActors)
	{
		if (CheckFolderPath(Actor, FolderInWorld))
		{
			FolderActors.Add(Actor);
			//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Actor->GetActorLabel());
		}
	}
	return FolderActors;
}

TMap<const FGuid, FName> FCreateMapUtils::GetOldActorBindMap(ULevelSequence * InSequence)
{
	TMap<const FGuid, FName> OldActorBindMap;

	if (InSequence != nullptr)
	{
		FAssetEditorManager::Get().OpenEditorForAsset(InSequence);
	}

	IAssetEditorInstance * AssetEditor = FAssetEditorManager::Get().FindEditorForAsset(InSequence, true);
	FLevelSequenceEditorToolkit * LevelSequenceEditor = (FLevelSequenceEditorToolkit *)AssetEditor;
	ISequencer *  Sequencer = LevelSequenceEditor->GetSequencer().Get();

	TArrayView<TWeakObjectPtr<UObject>> BindObjects;

	UMovieScene *  MyMovieScene = InSequence->MovieScene;
	int32 PosseableCount = MyMovieScene->GetPossessableCount();


	for (int i = 0; i < PosseableCount; i++)
	{
		FMovieScenePossessable Possable = MyMovieScene->GetPossessable(i);
		if (!Possable.GetParent().IsValid())
		{
			BindObjects = Sequencer->FindObjectsInCurrentSequence(Possable.GetGuid());
			if (BindObjects.Num())
			{
				OldActorBindMap.Add(Possable.GetGuid(), BindObjects[0]->GetFName());
			}
		}
	}

	return OldActorBindMap;
}


void FCreateMapUtils::FixActorBinds(ULevelSequence * InSequence, TMap<const FGuid, FName> OldActorBindMap)
{
	UMovieScene * MyMovieScene = InSequence->MovieScene;

	//Check Actor Bind Exist in Sequence 

	FString Folder = "VFX";
	TArray<AActor*> VFXActors = GetActorsInFolder(Folder);
	//TArray<AActor*> VFXActors = GWorld->GetCurrentLevel()->Actors;

	if (VFXActors.Num())
	{
		int32 PosseableCount = MyMovieScene->GetPossessableCount();
		TArray<FGuid> FindBindGuid, NotFindBindGuid;

		//Fix Actor Bind
		TMap<const FGuid, AActor*> ActorBindMap;
		for (int32 Index = 0; Index < PosseableCount; Index++)
		{
			FMovieScenePossessable Possable = MyMovieScene->GetPossessable(Index);

			bool bFind = false;
			if (!Possable.GetParent().IsValid())
			{
				if (OldActorBindMap.Contains(Possable.GetGuid()))
				{
					for (auto Actor : VFXActors)
					{
						if (OldActorBindMap[Possable.GetGuid()] == Actor->GetFName())
						{
							InSequence->BindPossessableObject(Possable.GetGuid(), *Actor, Actor->GetWorld());
							ActorBindMap.Add(Possable.GetGuid(), Actor);
							bFind = true;
							//FindBindGuid.Add(Possable.GetGuid());
							break;
						}
					}
				}
				if (!bFind)
				{
					NotFindBindGuid.Add(Possable.GetGuid());
				}
			}

		}

		// Fix Componenet Bind
		if (ActorBindMap.Num())
		{
			for (int32 Index = 0; Index < PosseableCount; Index++)
			{
				FMovieScenePossessable Possable = MyMovieScene->GetPossessable(Index);
				if (Possable.GetParent().IsValid())
				{
					FGuid ParentGuid = Possable.GetParent();
					bool bFind = false;

					if (ActorBindMap.Contains(ParentGuid))
					{
						AActor * Actor = ActorBindMap[ParentGuid];
						TSet<UActorComponent*> ActorComponents = Actor->GetComponents();
						for (auto it = ActorComponents.CreateIterator(); it; ++it)
						{
							UActorComponent * Component = *it;
							if (Component->GetName() == Possable.GetName())
							{
								InSequence->BindPossessableObject(Possable.GetGuid(), *Component, Actor->GetWorld());
								//FindBindGuid.Add(Possable.GetGuid());
								bFind = true;
								break;
							}
						}
					}
					if (!bFind)
					{
						NotFindBindGuid.Add(Possable.GetGuid());
					}
					
				}

			}
		}

		
		//Remove UnBind Guid
		if (NotFindBindGuid.Num())
		{
			for (auto Guid : NotFindBindGuid)
				MyMovieScene->RemovePossessable(Guid);
		}
	}

	
}

void FCreateMapUtils::CleanUpMap(FString& InMapPackage, FString& InSequencePackage,FString& OutputPath,bool bOverwrite)
{
	UEditorLoadingAndSavingUtils::LoadMap(*InMapPackage);
	
	FString Folder = "VFX";
	TArray<AActor*> VFXActors = GetActorsInFolder(Folder);


	//Select Actors In Folder
	if (VFXActors.Num())
	{

		FString OutMapName, OutSequenceName;
		OutMapName = FConvertPath::GetPackageName(InMapPackage);
		OutSequenceName = FConvertPath::GetPackageName(InSequencePackage);

		if (!bOverwrite)
		{
			if (InMapPackage.Contains(*OutputPath, ESearchCase::IgnoreCase, ESearchDir::FromStart))
				OutMapName += "_Clean";
			if (InSequencePackage.Contains(*OutputPath, ESearchCase::IgnoreCase, ESearchDir::FromStart))
				OutSequenceName += "_Clean";
		}

		//Duplicate Sequence Asset
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(FName("AssetTools"));
		IAssetTools& AssetTool = AssetToolsModule.Get();

		ULevelSequence * InSequence = LoadObject<ULevelSequence>(NULL, *InSequencePackage);

		TMap<const FGuid, FName> OldActorBindMap;
		OldActorBindMap = FCreateMapUtils::GetOldActorBindMap(InSequence);

		ULevelSequence * DuplicateSequence;
		FString DuplicateSequencePackage = "/Game/" + OutputPath + "/" + OutSequenceName;
		DuplicateSequence = LoadObject<ULevelSequence>(NULL, *DuplicateSequencePackage);

		if (DuplicateSequence == nullptr)
		{
			DuplicateSequence = (ULevelSequence*)AssetTool.DuplicateAsset(*OutSequenceName, *("/Game/" + OutputPath), InSequence);

			//Unbind Duplicate Sequence to Original Map
			UMovieScene * InMovieScene = DuplicateSequence->MovieScene;
			int32 InPosseableCount = InMovieScene->GetPossessableCount();
			for (int32 Index = 0; Index < InPosseableCount; Index++)
			{
				FMovieScenePossessable Possable = InMovieScene->GetPossessable(Index);
				DuplicateSequence->UnbindPossessableObjects(Possable.GetGuid());
			}
		}

		


		//Select Actors in VFX Folder
		GEditor->SelectNone(false, false, false);
		for (auto Actor : VFXActors)
		{
			GEditor->SelectActor(Actor, true, false, false, false);
		}

		//Copy And Paste Actors 
		GEditor->CopySelectedActorsToClipboard(GWorld, false, false, false);


		FEditorFileUtils::SaveCurrentLevel();
		GEditor->CreateNewMapForEditing();

		if (VFXActors.Num())
			GEditor->PasteSelectedActorsFromClipboard(GWorld, FText::FromString(""), EPasteTo::PT_OriginalLocation);

		SaveLevel(OutputPath, OutMapName);

				
		//Rebind Dupliacate Sequence to New Map
		FixActorBinds(DuplicateSequence,OldActorBindMap);

		UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
	}
}



int32 AutoRename(FString InFileName, int32 i,FString Ext)
{
	FString File = InFileName + "_" + FString::FromInt(i) + "." + Ext;

	if (!IFileManager::Get().FileExists(*File))
		return i;
	else
		return AutoRename(InFileName, i + 1,Ext);
}

int32 AutoRename(FAssetRenameData RenameData, int32 i, TArray<FAssetRenameData> RenameAssets)
{
	FString InPackage = RenameData.NewPackagePath + "/" + RenameData.NewName + "_" + FString::FromInt(i);
	
	if (RenameAssets.Num())
		for (auto Asset : RenameAssets)
		{
			FString ExistPackage = Asset.NewPackagePath + "/" + Asset.NewName;
			if (ExistPackage == InPackage)
				return AutoRename(RenameData, i + 1, RenameAssets);
		}

	return i;

	
}

TArray<FString> SetClassToBeSaved()
{
	TArray<FString> ToSaveClass;

	ToSaveClass.Add("Material");
	ToSaveClass.Add("MaterialInstance");
	ToSaveClass.Add("MaterialInstanceConstant");
	ToSaveClass.Add("MaterialInstanceDynamic");
	ToSaveClass.Add("MaterialFunction");
	ToSaveClass.Add("SkeletalMesh");

	return ToSaveClass;
}

TArray<FString> SetClassInMaterialFolder()
{
	TArray<FString> ClassInFolder;

	ClassInFolder.Add("Material");
	ClassInFolder.Add("MaterialInstanceConstant");
	ClassInFolder.Add("MaterialInstanceDynamic");
	ClassInFolder.Add("MaterialFunction");
	ClassInFolder.Add("MaterialParameterCollection");

	return ClassInFolder;
}

TArray<FString> SetClassInTextureFolder()
{
	TArray<FString> ClassInFolder;

	ClassInFolder.Add("Texture2D");

	return ClassInFolder;
}

TArray<FString> SetClassInParticleFolder()
{
	TArray<FString> ClassInFolder;

	ClassInFolder.Add("ParticleSystem");

	return ClassInFolder;
}

TArray<FString> SetClassInMeshFolder()
{
	TArray<FString> ClassInFolder;
	
	ClassInFolder.Add("StaticMesh");
	ClassInFolder.Add("SkeletalMesh");
	ClassInFolder.Add("Skeleton");
	ClassInFolder.Add("PhysicsAsset");

	return ClassInFolder;
}


// Save All Material Asset in Folder
 void FCreateMapUtils::SaveMaterialsInFolder(FString Folder)
{
	UObjectLibrary * ObjectLibrary = UObjectLibrary::CreateLibrary(UObject::StaticClass(), false, GIsEditor);
	if (ObjectLibrary)
	{
		ObjectLibrary->AddToRoot();
	}

	FString AssetsPath = "/Game/" + Folder;
	ObjectLibrary->LoadAssetDataFromPath(*AssetsPath);

	TArray<FAssetData> AssetsData;
	ObjectLibrary->GetAssetDataList(AssetsData);

	if (AssetsData.Num())
	{
		TArray<FString> ToSaveClass = SetClassToBeSaved();

		for (auto Asset : AssetsData)
		{
			UObject* Object = LoadObject<UObject>(NULL, *Asset.ToSoftObjectPath().ToString());
			FString ObjectClass = Object->GetClass()->GetName();
			int32 ClassIndex;
			if (ToSaveClass.Find(ObjectClass, ClassIndex))
				Object->MarkPackageDirty();
			
		}

		UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
	}
}

void FCreateMapUtils::MoveAssetInFolder(FString Folder, FString OutFolder, bool bMoveFolder)
{

	UObjectLibrary * ObjectLibrary = UObjectLibrary::CreateLibrary(UObject::StaticClass(), false, GIsEditor);
	if (ObjectLibrary)
	{
		ObjectLibrary->AddToRoot();
	}

	FString AnimAssetsPath = "/Game/" + Folder;
	ObjectLibrary->LoadAssetDataFromPath(*AnimAssetsPath);

	TArray<FAssetData> AssetsData;
	ObjectLibrary->GetAssetDataList(AssetsData);


	if (AssetsData.Num())
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(FName("AssetTools"));
		IAssetTools& AssetTool = AssetToolsModule.Get();
		
		TArray<FAssetRenameData> RenameAssets;
		for (auto Asset : AssetsData)
		{
			UObject* Object = LoadObject<UObject>(NULL, *Asset.ToSoftObjectPath().ToString());
			FString Package = Asset.PackageName.ToString();

			FString OutPath, OutPackage;

			FString ObjectClass = Object->GetClass()->GetName();
			
			if (bMoveFolder)
			{
				OutPath = "/Game/" + OutFolder + "/";
				OutPackage = OutPath + Package.RightChop(6);
			}
			else
			{
				FString Class = Object->GetClass()->GetName();

				TArray<FString> InTextureFolderClass, InMaterialFolderClass, InMeshFolderClass, InParticleFolderClass;
				InTextureFolderClass = SetClassInTextureFolder();
				InMaterialFolderClass = SetClassInMaterialFolder();
				InMeshFolderClass = SetClassInMeshFolder();
				InParticleFolderClass = SetClassInParticleFolder();

				int32 FindIndex;

				if (InTextureFolderClass.Find(Class, FindIndex))
					OutPath = "/Game/" + OutFolder + "/" + "Textures/";
				else if (InMaterialFolderClass.Find(Class, FindIndex))
					OutPath = "/Game/" + OutFolder + "/" + "Materials/";
				else if (InMeshFolderClass.Find(Class, FindIndex))
					OutPath = "/Game/" + OutFolder + "/" + "Meshes/";
				else if (InParticleFolderClass.Find(Class, FindIndex))
					OutPath = "/Game/" + OutFolder + "/" + "Particles/";
				else
					OutPath = "/Game/" + OutFolder + "/" + "Others/";

				OutPackage = OutPath + FConvertPath::GetPackageName(Package);

				FString Ext = "uasset";
				FString OutFile = FConvertPath::ToAbsolutePath(OutPackage, true, Ext);
				if (IFileManager::Get().FileExists(*OutFile))
				{
					int32 FileVersion = AutoRename(FConvertPath::ToAbsolutePath(OutPackage), 0, Ext);
					OutPackage = OutPath + FConvertPath::GetPackageName(Package) + "_" + FString::FromInt(FileVersion);
				}
			}
			
			FString OutPackageName = FConvertPath::GetPackageName(OutPackage);
			FString OutPackagePath = FConvertPath::GetPackagePath(OutPackage);

		
			FAssetRenameData RenameData = FAssetRenameData(Object, *OutPackagePath, *OutPackageName);
			//TArray<FAssetRenameData> RenameAssets;
			bool bFind = false;
			if (RenameAssets.Num())
				for (auto Asset : RenameAssets)
				{
					FString ExistPackage = Asset.NewPackagePath + "/" + Asset.NewName;
					if (ExistPackage == OutPackage)
					{
						bFind = true;
						break;
					}
				}
			
			if (bFind)
			{
				int32 Version = AutoRename(RenameData, 0, RenameAssets);
				FString NewOutPackageName = OutPackageName + "_" + FString::FromInt(Version);
				RenameData = FAssetRenameData(Object, *OutPackagePath, *NewOutPackageName);
			}


			RenameAssets.Add(RenameData);
			//AssetTool.RenameAssets(RenameAssets);
		}
		AssetTool.RenameAssets(RenameAssets);
	}
}

void FCreateMapUtils::FixRedirectorsInFolder(FString Folder)
{
	UObjectLibrary * AnimLibrary = UObjectLibrary::CreateLibrary(UObjectRedirector::StaticClass(), false, GIsEditor);
	if (AnimLibrary)
	{
		AnimLibrary->AddToRoot();
	}

	FString AnimAssetsPath = "/Game/" + Folder;
	AnimLibrary->LoadAssetDataFromPath(*AnimAssetsPath);

	TArray<FAssetData> AssetsData;
	AnimLibrary->GetAssetDataList(AssetsData);

	TArray<UObjectRedirector*> ObjectRedirectors;

	//Find and Fix up Object Radirector 
	if (AssetsData.Num())
	{
		for (auto Asset : AssetsData)
		{

			UObjectRedirector* Redirector = LoadObject<UObjectRedirector>(NULL, *Asset.ToSoftObjectPath().ToString());
			ObjectRedirectors.Add(Redirector);
		}

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(FName("AssetTools"));
		IAssetTools& AssetTool = AssetToolsModule.Get();
		AssetTool.FixupReferencers(ObjectRedirectors);
	}
}



void FCreateMapUtils::MoveAssetInCurrentFolder(FString Folder, FString OutFolder, bool bMoveFolder)
{

	TArray<FString> FoundFiles;
	FString AbsolutePath = FPaths::ProjectContentDir() + Folder;
	IFileManager::Get().FindFiles(FoundFiles, *AbsolutePath, TEXT(".uasset"));

	TArray<FAssetRenameData> RenameAssets;
	if (FoundFiles.Num())
		for (auto File : FoundFiles)
		{
			FString AbsoluteFilePath;
			if (Folder == "")
				AbsoluteFilePath = AbsolutePath + File;
			else
				AbsoluteFilePath = AbsolutePath + "/" + File;
			FString Package = FConvertPath::ToRelativePath(AbsoluteFilePath, false);

			UObject * Object = LoadObject<UObject>(NULL, *Package);

			FString OutPath, OutPackage;
			if (bMoveFolder)
			{
				OutPath = "/Game/" + OutFolder + "/";
				OutPackage = OutPath + Package.RightChop(6);
			}
			else
			{
				FString Class = Object->GetClass()->GetName();

				TArray<FString> InTextureFolderClass, InMaterialFolderClass, InMeshFolderClass, InParticleFolderClass;
				InTextureFolderClass = SetClassInTextureFolder();
				InMaterialFolderClass = SetClassInMaterialFolder();
				InMeshFolderClass = SetClassInMeshFolder();
				InParticleFolderClass = SetClassInParticleFolder();

				int32 FindIndex;

				if (InTextureFolderClass.Find(Class, FindIndex))
					OutPath = "/Game/" + OutFolder + "/" + "Textures/";
				else if (InMaterialFolderClass.Find(Class, FindIndex))
					OutPath = "/Game/" + OutFolder + "/" + "Materials/";
				else if (InMeshFolderClass.Find(Class, FindIndex))
					OutPath = "/Game/" + OutFolder + "/" + "Meshes/";
				else if (InParticleFolderClass.Find(Class, FindIndex))
					OutPath = "/Game/" + OutFolder + "/" + "Particles/";
				else
					OutPath = "/Game/" + OutFolder + "/" + "Others/";

				OutPackage = OutPath + FConvertPath::GetPackageName(Package);

				FString Ext = "uasset";
				FString OutFile = FConvertPath::ToAbsolutePath(OutPackage, true, Ext);
				if (IFileManager::Get().FileExists(*OutFile))
				{
					int32 FileVersion = AutoRename(FConvertPath::ToAbsolutePath(OutPackage), 0,Ext);
					OutPackage = OutPath + FConvertPath::GetPackageName(Package) + "_" + FString::FromInt(FileVersion);
				}

			}

			FString OutPackageName = FConvertPath::GetPackageName(OutPackage);
			FString OutPackagePath = FConvertPath::GetPackagePath(OutPackage);

			FAssetRenameData RenameData = FAssetRenameData(Object, *OutPackagePath, *OutPackageName);
			RenameAssets.Add(RenameData);

		}


	if (RenameAssets.Num())
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(FName("AssetTools"));
		IAssetTools& AssetTool = AssetToolsModule.Get();
		AssetTool.RenameAssets(RenameAssets);
	}

}