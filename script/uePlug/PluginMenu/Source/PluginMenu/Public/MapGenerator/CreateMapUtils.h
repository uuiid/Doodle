#pragma once
#include "CoreMinimal.h"

#include "MapGenerator/ConvertPath.h"
#include "MapGenerator/DataType.h"


class FCreateMapUtils
{
public:
	/*Save Level 
	@LevelPackagePath:Relative Path Without "/Game/"
	@LevelName : Level Name
	*/
	static void SaveLevel(FString& LevelPackagePath, FString& LevelName);

	//Get Referenced Level Sequecne of Map
	static TArray<FString> GetSequenceReferencers(FString& MapPackage);

	//Get Actors in The Folder of World Outliner
	static TArray<AActor*> GetActorsInFolder(FString& FolderInWorld);

	//Add Streaming Level
	static void AddStreamingLevel(FString& StreamingLevelMapPackage);

	//Remove Streaming Level
	static void RemoveStreamingLevel(FString& StreamingLevelMapPackage);

	//Get Old Actor BindMap
	static TMap<const FGuid, FName>GetOldActorBindMap(ULevelSequence * InSequence);

	//Fix Actor Bind In Level Sequence
	static void FixActorBinds(ULevelSequence * InSequence, TMap<const FGuid, FName> OldActorBindMap);

	

	/*Copy Actors In "VFX" Folder of World Outliner to a NewMap,Remove Unbind Posseable in Level Sequence
	@InMapPackage : Map to Clean up
	@InSequencePackage : Level Sequence to Clean up
	@OutputPath : NewMap and NewSequence Saved Path
	@bOverwrite : Can NewMap and NewSequence Overwrite OldMap and OldSequence
	*/
	static void CleanUpMap(FString& InMapPackage, FString& InSequencePackage,FString& OutputPath,bool bOverwrite);

	
	

	/*Move Assets In The Folder of ContentBrowser
	@SourceFolder: A Folder of ContentBrowser
	@DestFolder : Destination Folder
	@bMoveFolder : Maintain File Sturture or Not
	*/
	static void MoveAssetInFolder(FString SourceFolder, FString DestFolder, bool bMoveFolder);
	static void MoveAssetInCurrentFolder(FString SourceFolder, FString DestFolder, bool bMoveFolder);

	//Fix Redirector
	static void FixRedirectorsInFolder(FString SourceFolder);

	// Save All Material Asset in Folder
	static void SaveMaterialsInFolder(FString Folder);


public:
	static TArray<FString> ToSaveClass;
};

