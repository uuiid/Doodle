// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleAutoAnimationCommandlet.h"
#include "Engine.h"
#include "Engine/Light.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "CineCameraActor.h"
#include "Tracks/MovieSceneSpawnTrack.h"
#include "Sections/MovieSceneSpawnSection.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "MovieSceneToolHelpers.h"
#include "LevelSequenceActor.h"
#include "FbxImporter.h"
#include "MovieSceneToolsUserSettings.h"
#include "AssetImportTask.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Animation/SkeletalMeshActor.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Tracks/MovieSceneLevelVisibilityTrack.h"
#include "Sections/MovieSceneLevelVisibilitySection.h"
#include "MoviePipelineQueueSubsystem.h"
#include "MovieRenderPipelineSettings.h"
#include "MoviePipelineOutputSetting.h"
#include "MoviePipelinePIEExecutor.h"
#include "MoviePipelineInProcessExecutor.h"
#include "MoviePipelineUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "LevelSequencePlayer.h"
#include "Factories/WorldFactory.h"
#include "MoviePipelineImageSequenceOutput.h"
#include "MoviePipelineDeferredPasses.h"
#include "Engine/LevelStreamingDynamic.h"
#include "EditorLevelUtils.h"
#include "Doodle/Abc/DoodleAlembicImportFactory.h"
///-------------
#include "Doodle/Abc/DoodleAbcImportSettings.h"
#include "GeometryCache.h"       //
#include "GeometryCacheActor.h"
#include "GeometryCacheComponent.h"
#include "MovieSceneGeometryCacheTrack.h"
#include "AbcImportSettings.h"
#include "AlembicImportFactory.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Components/LightComponent.h"
#include "Doodle/DoodleAssetsPreview.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
#include "Animation/AnimBoneCompressionSettings.h"
//---

#include "Components/DirectionalLightComponent.h"
#include "MoviePipelineAntiAliasingSetting.h"
#include "MoviePipelineGameOverrideSetting.h"
#include "MoviePipelineConsoleVariableSetting.h"
#include "Kismet/GameplayStatics.h"

#include "FileHelpers.h"

UDoodleAutoAnimationCommandlet::UDoodleAutoAnimationCommandlet()
	: Super()
{
	LogToConsole = true;
}

int32 UDoodleAutoAnimationCommandlet::Main(const FString& Params)
{
	TArray<FString> CmdLineTokens;
	TArray<FString> CmdLineSwitches;
	//------------------
	TMap<FString, FString> ParamsMap;
	ParseCommandLine(*Params, CmdLineTokens, CmdLineSwitches, ParamsMap);
	//--------------
	if (const FString& Key = TEXT("Params"); ParamsMap.Contains(Key))
	{
		RunAutoLight(ParamsMap[Key]);
		return 0;
	}
	else if (const FString& Key2 = TEXT("Check"); ParamsMap.Contains(Key2))
	{
		RunCheckFiles(ParamsMap[Key2]);
		return 0;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("No params field in cmd arguments"));
		return -1;
	}
}

void UDoodleAutoAnimationCommandlet::RunCheckFiles(const FString& InCondigPath)
{
	//--------------------
	TSharedPtr<FJsonObject> JsonObject;
	if (FString JsonString; FFileHelper::LoadFileToString(JsonString, *InCondigPath))
	{
		const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
		FJsonSerializer::Deserialize(JsonReader, JsonObject);
	}

	//--------------------
	RenderMapPath = JsonObject->GetStringField(TEXT("render_map"));
	CreateMapPath = JsonObject->GetStringField(TEXT("create_map"));
	OriginalMapPath = JsonObject->HasField(TEXT("original_map")) ? JsonObject->GetStringField(TEXT("original_map")) : FString{};
	DestinationPath = JsonObject->GetStringField(TEXT("out_file_dir"));
	ImportPath = JsonObject->GetStringField(TEXT("import_dir"));

	SequencePath = JsonObject->GetStringField(TEXT("level_sequence_import"));
	MoviePipelineConfigPath = JsonObject->GetStringField(TEXT("movie_pipeline_config"));

	if (JsonObject->HasField(TEXT("import_files")))
	{
		const TSharedPtr<FJsonObject>& JsonFiles = JsonObject->GetObjectField(TEXT("import_files"));
		const FString Path = JsonFiles->GetStringField(TEXT("path"));
		const FString Type = JsonFiles->GetStringField(TEXT("type"));
		if (FPaths::FileExists(Path))
		{
			EImportFilesType2 Type2 = EImportFilesType2::Camera;
			if (Type == TEXT("cam")) Type2 = EImportFilesType2::Camera;
			else if (Type == TEXT("char")) Type2 = EImportFilesType2::Character;
			else if (Type == TEXT("geo")) Type2 = EImportFilesType2::Geometry;
			ImportFiles.Add(FImportFiles2{Type2, Path});
		}
	}

	if (JsonObject->GetStringField(TEXT("check_type")) == TEXT("char_")) CheckFileType = ECheckFileType::Character;
	else if (JsonObject->GetStringField(TEXT("check_type")) == TEXT("scene")) CheckFileType = ECheckFileType::Scene;
	else CheckFileType = ECheckFileType::Scene;
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();


	//--------------------
	DeleteAsseet(RenderMapPath);
	DeleteAsseet(CreateMapPath);
	DeleteAsseet(SequencePath);
	UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);

	// 创建主要的关卡和关卡序列
	OnCreateSequence();
	OnCreateSequenceWorld();
	// 加载渲染关卡
	if (!OriginalMapPath.IsEmpty())
	{
		EditorAssetSubsystem->DuplicateAsset(OriginalMapPath, RenderMapPath);
		TheRenderWorld = LoadObject<UWorld>(nullptr, *RenderMapPath);
		EditorAssetSubsystem->SaveLoadedAsset(TheRenderWorld);
		// FString Filename;
		// FPackageName::TryConvertLongPackageNameToFilename(RenderMapPath, Filename);
		UPackage* Package = LoadPackage(NULL, *RenderMapPath, 0);
		TheRenderWorld = UWorld::FindWorldInPackage(Package);


		// Clean up any previous world.  The world should have already been saved
		UWorld* ExistingWorld = GEditor->GetEditorWorldContext().World();

		GEngine->DestroyWorldContext(ExistingWorld);
		ExistingWorld->DestroyWorld(true, TheRenderWorld);

		GWorld = TheRenderWorld;

		TheRenderWorld->WorldType = EWorldType::Editor;

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(TheRenderWorld->WorldType);
		WorldContext.SetCurrentWorld(TheRenderWorld);

		// add the world to the root set so that the garbage collection to delete replaced actors doesn't garbage collect the whole world
		TheRenderWorld->AddToRoot();

		// initialize the levels in the world
		TheRenderWorld->InitWorld(UWorld::InitializationValues().AllowAudioPlayback(false));
		TheRenderWorld->GetWorldSettings()->PostEditChange();
		TheRenderWorld->UpdateWorldComponents(true, false);

		// UGameplayStatics::OpenLevel(TheRenderWorld, FName{RenderMapPath});
		UGameplayStatics::FlushLevelStreaming(TheRenderWorld);
	}
	else
	{
		UWorldFactory* Factory = NewObject<UWorldFactory>();
		UPackage* Pkg = CreatePackage(*RenderMapPath);
		Pkg->FullyLoad();
		Pkg->MarkPackageDirty();
		const FString PackagePath = FPackageName::GetLongPackagePath(RenderMapPath);
		const FString BaseFileName = FPaths::GetBaseFilename(RenderMapPath);
		//---------------
		const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		TheRenderWorld = CastChecked<UWorld>(AssetToolsModule.Get().CreateAsset(BaseFileName, PackagePath, UWorld::StaticClass(), Factory));
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().AssetCreated(TheRenderWorld);
		//------------------------
		TheRenderWorld->Modify();
		EditorAssetSubsystem->SaveLoadedAsset(TheRenderWorld, false);
	}
	// 创建标准环境
	if (CheckFileType == ECheckFileType::Scene) ClearAllLight();
	OnCreateCheckLight();

	// 创建主要定序器
	switch (CheckFileType)
	{
	case ECheckFileType::Character:
		OnBuildCheckCharacter();
		break;
	case ECheckFileType::Scene:
		OnBuildCheckScene();
		break;
	default:
		break;
	}

	AddSequenceWorldToRenderWorld();
	TArray<UObject*> AllLevels{TheLevelSequence, TheSequenceWorld, TheRenderWorld};
	for (ULevelStreaming* StreamingLevel : TheRenderWorld->GetStreamingLevels())
	{
		if (const ULevel* Level = StreamingLevel->GetLoadedLevel(); Level)
		{
			AllLevels.Add(StreamingLevel->GetLoadedLevel());
			Level->GetPackage()->MarkPackageDirty();
		}
	}

	EditorAssetSubsystem->SaveLoadedAssets(AllLevels, false);
	UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
	// 创建渲染配置
	OnSaveReanderConfig();
}


void UDoodleAutoAnimationCommandlet::RunAutoLight(const FString& InCondigPath)
{
	FixMaterialProperty();

	//--------------------
	TSharedPtr<FJsonObject> JsonObject;
	if (FString JsonString; FFileHelper::LoadFileToString(JsonString, *InCondigPath))
	{
		const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
		FJsonSerializer::Deserialize(JsonReader, JsonObject);
	}
	//--------------------
	OriginalMapPath = JsonObject->GetStringField(TEXT("original_map"));
	RenderMapPath = JsonObject->GetStringField(TEXT("render_map"));
	CreateMapPath = JsonObject->GetStringField(TEXT("create_map"));
	ImportPath = JsonObject->GetStringField(TEXT("import_dir"));
	EffectSequencePath = JsonObject->GetStringField(TEXT("level_sequence_vfx"));
	EffectMapPath = JsonObject->GetStringField(TEXT("vfx_map"));
	SequencePath = JsonObject->GetStringField(TEXT("level_sequence"));
	L_Start = FFrameNumber(JsonObject->GetIntegerField(TEXT("begin_time")));
	L_End = FFrameNumber(JsonObject->GetIntegerField(TEXT("end_time")));
	DestinationPath = JsonObject->GetStringField(TEXT("out_file_dir"));
	MoviePipelineConfigPath = JsonObject->GetStringField(TEXT("movie_pipeline_config"));

	for (TArray<TSharedPtr<FJsonValue>> JsonFiles = JsonObject->GetArrayField(TEXT("files")); const TSharedPtr<FJsonValue>& JsonFile : JsonFiles)
	{
		TSharedPtr<FJsonObject> Obj = JsonFile->AsObject();
		const FString Path = Obj->GetStringField(TEXT("path"));
		const FString Type = Obj->GetStringField(TEXT("type"));
		if (!FPaths::FileExists(Path)) continue;
		EImportFilesType2 Type2 = EImportFilesType2::Camera;
		if (Type == TEXT("cam")) Type2 = EImportFilesType2::Camera;
		else if (Type == TEXT("char")) Type2 = EImportFilesType2::Character;
		else if (Type == TEXT("geo")) Type2 = EImportFilesType2::Geometry;
		ImportFiles.Add(FImportFiles2{Type2, Path});
	}

	//--------------------
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	if (!EditorAssetSubsystem->DoesAssetExist(OriginalMapPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Original Map Asset File Not Found!"));
	}
	//--------------------
	DeleteAsseet(RenderMapPath);
	DeleteAsseet(CreateMapPath);
	DeleteAsseet(EffectSequencePath);
	DeleteAsseet(EffectMapPath);
	DeleteAsseet(SequencePath);

	// 创建主要的关卡和关卡序列
	OnCreateSequence();
	OnCreateSequenceWorld();
	OnCreateDirectionalLight();
	/// 创建特效关卡和关卡序列
	OnCreateEffectSequenceWorld();
	OnCreateEffectSequence();

	OnBuildSequence();
	//---------------------
	EditorAssetSubsystem->DuplicateAsset(OriginalMapPath, RenderMapPath);
	TheRenderWorld = LoadObject<UWorld>(nullptr, *RenderMapPath);
	AddSequenceWorldToRenderWorld();

	EditorAssetSubsystem->SaveLoadedAssets({TheLevelSequence, TheRenderWorld});
	//-----------------
	OnSaveReanderConfig();
}

void UDoodleAutoAnimationCommandlet::AddSequenceWorldToRenderWorld()
{
	ULevelStreaming* TempStreamingLevel = NewObject<ULevelStreaming>(TheRenderWorld, ULevelStreamingDynamic::StaticClass(), NAME_None, RF_NoFlags);
	TempStreamingLevel->SetWorldAsset(TheSequenceWorld);
	TheRenderWorld->AddStreamingLevel(TempStreamingLevel);

	//---------------
	UMovieSceneLevelVisibilityTrack* NewTrack = TheLevelSequence->GetMovieScene()->FindTrack<UMovieSceneLevelVisibilityTrack>();
	if (NewTrack)
	{
		TheLevelSequence->GetMovieScene()->RemoveTrack(*Cast<UMovieSceneTrack>(NewTrack));
	}
	NewTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneLevelVisibilityTrack>();
	UMovieSceneLevelVisibilitySection* NewSection = CastChecked<UMovieSceneLevelVisibilitySection>(NewTrack->CreateNewSection());
	NewSection->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
	NewTrack->AddSection(*NewSection);
	//-----------------
	TArray<FName> LevelNames;
	for (ULevelStreaming* StreamingLevel : TheRenderWorld->GetStreamingLevels())
	{
		if (StreamingLevel && !LevelNames.Contains(StreamingLevel->GetWorldAssetPackageFName()))
		{
			LevelNames.Add(StreamingLevel->GetWorldAssetPackageFName());
		}
	}
	LevelNames.Add(FName{RenderMapPath});
	//-------------
	if (!EffectMapPath.IsEmpty()) LevelNames.Add(FName{EffectMapPath});
	NewSection->SetLevelNames(LevelNames);
}


void UDoodleAutoAnimationCommandlet::OnCreateEffectSequence()
{
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const FString AssetName = FPaths::GetBaseFilename(EffectSequencePath);

	UPackage* Package = CreatePackage(*EffectSequencePath);
	ULevelSequence* EffectLevelSequence = NewObject<ULevelSequence>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	EffectLevelSequence->Initialize();
	IAssetRegistry::GetChecked().AssetCreated(EffectLevelSequence);
	Package->Modify();
	//--------------------------
	EffectLevelSequence->GetMovieScene()->SetDisplayRate(Rate);
	EffectLevelSequence->GetMovieScene()->SetTickResolutionDirectly(Rate);
	//--------------------
	EffectLevelSequence->GetMovieScene()->SetWorkingRange((L_Start - 30 - Offset) / Rate, (L_End + 30) / Rate);
	EffectLevelSequence->GetMovieScene()->SetViewRange((L_Start - 30 - Offset) / Rate, (L_End + 30) / Rate);
	EffectLevelSequence->GetMovieScene()->SetPlaybackRange(TRange<FFrameNumber>{L_Start - Offset, L_End + 1}, true);
	EffectLevelSequence->GetMovieScene()->Modify();

	//----------------------------------------
	UMovieSceneSubTrack* NewTrack1 = TheLevelSequence->GetMovieScene()->FindTrack<UMovieSceneSubTrack>();
	if (NewTrack1)
	{
		TheLevelSequence->GetMovieScene()->RemoveTrack(*Cast<UMovieSceneTrack>(NewTrack1));
	}
	NewTrack1 = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSubTrack>();
	UMovieSceneSubSection* NewSection1 = CastChecked<UMovieSceneSubSection>(NewTrack1->CreateNewSection());
	NewSection1->SetSequence(EffectLevelSequence);
	NewSection1->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
	NewTrack1->AddSection(*NewSection1);
	//---------------------
	TheLevelSequence->Modify();
	EditorAssetSubsystem->SaveLoadedAsset(EffectLevelSequence);
}

void UDoodleAutoAnimationCommandlet::DeleteAsseet(const FString& InPath)
{
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	if (EditorAssetSubsystem->DoesAssetExist(InPath))
	{
		EditorAssetSubsystem->DeleteAsset(InPath);
	}
}

void UDoodleAutoAnimationCommandlet::OnCreateEffectSequenceWorld()
{
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();

	UWorldFactory* Factory = NewObject<UWorldFactory>();
	UPackage* Pkg = CreatePackage(*EffectMapPath);
	Pkg->FullyLoad();
	Pkg->MarkPackageDirty();
	const FString PackagePath = FPackageName::GetLongPackagePath(EffectMapPath);
	const FString BaseFileName = FPaths::GetBaseFilename(EffectMapPath);
	//---------------
	const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	UObject* TempObject = AssetToolsModule.Get().CreateAsset(BaseFileName, PackagePath, UWorld::StaticClass(), Factory);
	UWorld* EffectSequenceWorld = Cast<UWorld>(TempObject);
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().AssetCreated(EffectSequenceWorld);
	//------------------------
	EffectSequenceWorld->Modify();
	EditorAssetSubsystem->SaveLoadedAsset(EffectSequenceWorld);
}

void UDoodleAutoAnimationCommandlet::OnCreateSequence()
{
	const FString AssetName = FPaths::GetBaseFilename(SequencePath);

	UPackage* Package = CreatePackage(*SequencePath);
	TheLevelSequence = NewObject<ULevelSequence>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	TheLevelSequence->Initialize();
	IAssetRegistry::GetChecked().AssetCreated(TheLevelSequence);
	Package->Modify();

	//--------------------------
	TheLevelSequence->GetMovieScene()->SetDisplayRate(Rate);
	TheLevelSequence->GetMovieScene()->SetTickResolutionDirectly(Rate);
	//--------------------
	TheLevelSequence->GetMovieScene()->SetWorkingRange((L_Start - 30 - Offset) / Rate, (L_End + 30) / Rate);
	TheLevelSequence->GetMovieScene()->SetViewRange((L_Start - 30 - Offset) / Rate, (L_End + 30) / Rate);
	TheLevelSequence->GetMovieScene()->SetPlaybackRange(TRange<FFrameNumber>{L_Start - Offset, L_End + 1}, true);
	TheLevelSequence->GetMovieScene()->Modify();
}

void UDoodleAutoAnimationCommandlet::OnCreateSequenceWorld()
{
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();

	UWorldFactory* Factory = NewObject<UWorldFactory>();
	UPackage* Pkg = CreatePackage(*CreateMapPath);
	Pkg->FullyLoad();
	Pkg->MarkPackageDirty();
	const FString PackagePath = FPackageName::GetLongPackagePath(CreateMapPath);
	const FString BaseFileName = FPaths::GetBaseFilename(CreateMapPath);
	//---------------
	const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	TheSequenceWorld = CastChecked<UWorld>(AssetToolsModule.Get().CreateAsset(BaseFileName, PackagePath, UWorld::StaticClass(), Factory));
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().AssetCreated(TheSequenceWorld);
	//------------------------
	TheSequenceWorld->Modify();
	EditorAssetSubsystem->SaveLoadedAsset(TheSequenceWorld);
	TheSequenceWorld = LoadObject<UWorld>(nullptr, *CreateMapPath);
}

void UDoodleAutoAnimationCommandlet::OnCreateDirectionalLight()
{
	//-----------------------
	DirectionalLight1 = TheSequenceWorld->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator::ZeroRotator);
	DirectionalLight1->SetBrightness(2.0f);
	DirectionalLight1->GetLightComponent()->SetLightingChannels(false, true, false);

	DirectionalLight2 = TheSequenceWorld->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator::ZeroRotator);
	DirectionalLight2->SetBrightness(7.0f);
	DirectionalLight2->GetLightComponent()->SetLightingChannels(false, true, false);

	// 设置可移动性
	DirectionalLight1->SetMobility(EComponentMobility::Movable);
	DirectionalLight2->SetMobility(EComponentMobility::Movable);

	// 设置源角度
	DirectionalLight1->GetComponent()->SetLightSourceAngle(15);
	DirectionalLight2->GetComponent()->SetLightSourceAngle(15);

	// 设置间接光强度
	DirectionalLight1->GetComponent()->SetIndirectLightingIntensity(0.0f);
	DirectionalLight2->GetComponent()->SetIndirectLightingIntensity(0.0f);

	// 设置体积散射光强度
	DirectionalLight1->GetComponent()->SetVolumetricScatteringIntensity(0.0f);
	DirectionalLight2->GetComponent()->SetVolumetricScatteringIntensity(0.0f);

	// 设置大气太阳光
	DirectionalLight1->GetComponent()->SetAtmosphereSunLight(false);
	DirectionalLight2->GetComponent()->SetAtmosphereSunLight(false);

	// 设置影响半透明
	DirectionalLight1->GetComponent()->SetAffectTranslucentLighting(false);
	DirectionalLight2->GetComponent()->SetAffectTranslucentLighting(false);

	// 取消投射阴影
	DirectionalLight1->GetComponent()->SetCastShadows(false);
	// 高光度范围
	DirectionalLight1->GetComponent()->SetSpecularScale(0.2f);
}

void UDoodleAutoAnimationCommandlet::OnCreateCheckLight()
{
	// 只添加标准灯光环境
	TheSequenceWorld->SpawnActor<ADoodleAssetsPreview>(FVector::ZeroVector, FRotator::ZeroRotator);
}

void UDoodleAutoAnimationCommandlet::ClearAllLight()
{
	UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	TArray<AActor*> Actors;
	for (TActorIterator<ALight> LightItr(TheRenderWorld); LightItr; ++LightItr)
	{
		Actors.Add(*LightItr);
	}
	for (TActorIterator<APostProcessVolume> PoseItr(TheRenderWorld); PoseItr; ++PoseItr)
	{
		Actors.Add(*PoseItr);
	}
	EditorActorSubsystem->DestroyActors(Actors);
	for (auto i = 0; i < 5; ++i)
	{
		GEngine->ForceGarbageCollection(true);
		GEngine->PerformGarbageCollectionAndCleanupActors();
		GEngine->ConditionalCollectGarbage();
		CommandletHelpers::TickEngine(TheRenderWorld);
	}
}


void UDoodleAutoAnimationCommandlet::ImportCamera(const FString& InFbxPath) const
{
	UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	TArray<FMovieSceneBinding> Bindings = TheLevelSequence->GetMovieScene()->GetBindings();
	for (const FMovieSceneBinding& Bind : Bindings)
	{
		if (Bind.GetName().Contains(TEXT("Camera")))
		{
			if (!TheLevelSequence->GetMovieScene()->RemovePossessable(Bind.GetObjectGuid()))
			{
				TheLevelSequence->GetMovieScene()->RemoveSpawnable(Bind.GetObjectGuid());
			}
		}
	}
	//--------------------
	AActor* TempActor = EditorActorSubsystem->SpawnActorFromClass(ACineCameraActor::StaticClass(), FVector::ZAxisVector, FRotator::ZeroRotator, false);
	ACineCameraActor* CameraActor = CastChecked<ACineCameraActor>(TheLevelSequence->MakeSpawnableTemplateFromInstance(*TempActor, TempActor->GetFName()));
	CameraActor->GetCineCameraComponent()->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
	FGuid CameraGuid = TheLevelSequence->GetMovieScene()->AddSpawnable(CameraActor->GetName(), *CameraActor);
	//---------------
	UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(CameraGuid);
	UMovieSceneSpawnSection* L_MovieSceneSpawnSection = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
	L_MovieSceneSpawnSection->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
	L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);
	//------------------------
	MovieSceneToolHelpers::CreateCameraCutSectionForCamera(TheLevelSequence->GetMovieScene(), CameraGuid, L_Start);
	UMovieSceneTrack* CameraCutTrack = TheLevelSequence->GetMovieScene()->GetCameraCutTrack();
	UMovieSceneCameraCutSection* CutSection;
	if (CameraCutTrack->GetAllSections().Num() <= 0)
	{
		CutSection = Cast<UMovieSceneCameraCutSection>(CameraCutTrack->CreateNewSection());
		CameraCutTrack->AddSection(*CutSection);
	}
	else
	{
		CutSection = CastChecked<UMovieSceneCameraCutSection>(CameraCutTrack->GetAllSections().Top());
	}
	//--------------------
	CutSection->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
	CutSection->SetCameraGuid(CameraGuid);
	FMovieSceneObjectBindingID BindingID = CutSection->GetCameraBindingID();
	//------------------
	FString CameraLabel = CameraActor->GetActorNameOrLabel();
	TMap<FGuid, FString> L_Map{};
	L_Map.Add(BindingID.GetGuid(), CameraLabel);
	//---------------
	ALevelSequenceActor* L_LevelSequenceActor{};
	ULevelSequencePlayer* L_LevelSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GWorld->PersistentLevel, TheLevelSequence, FMovieSceneSequencePlaybackSettings{}, L_LevelSequenceActor);
	L_LevelSequenceActor->InitializePlayer();
	L_LevelSequencePlayer->Play();
	//-----------------------
	FFBXInOutParameters InOutParams;
	UMovieSceneUserImportFBXSettings* L_ImportFBXSettings = GetMutableDefault<UMovieSceneUserImportFBXSettings>();
	L_ImportFBXSettings->bMatchByNameOnly = false;
	L_ImportFBXSettings->bForceFrontXAxis = false;
	L_ImportFBXSettings->bConvertSceneUnit = false;
	L_ImportFBXSettings->bCreateCameras = false;
	L_ImportFBXSettings->bReplaceTransformTrack = true;
	L_ImportFBXSettings->bReduceKeys = false;
	MovieSceneToolHelpers::ReadyFBXForImport(InFbxPath, L_ImportFBXSettings, InOutParams);
	//---------------------
	UnFbx::FFbxImporter* FbxImporter = UnFbx::FFbxImporter::GetInstance();
	FbxImporter->ImportFromFile(*InFbxPath, FPaths::GetExtension(InFbxPath));
	MovieSceneToolHelpers::ImportFBXCameraToExisting(FbxImporter, TheLevelSequence, L_LevelSequencePlayer, BindingID.GetRelativeSequenceID(), L_Map, false, false);
	//---------------------
	bool bValid = MovieSceneToolHelpers::ImportFBXIfReady(GWorld, TheLevelSequence, L_LevelSequencePlayer, BindingID.GetRelativeSequenceID(), L_Map, L_ImportFBXSettings, InOutParams);
	//----------------
	TempActor->Destroy();
	L_LevelSequenceActor->Destroy();
	FbxImporter->ReleaseScene();
	//----------------------------
	Bindings = TheLevelSequence->GetMovieScene()->GetBindings();
	TArray<FRotator> MainLightRot{};
	MainLightRot.Reserve(FMath::Abs((L_End - L_Start).Value) + 1);
	for (const FMovieSceneBinding& Bind : Bindings)
	{
		if (Bind.GetObjectGuid() == BindingID.GetGuid())
		{
			UMovieScene3DTransformTrack* TransformTrack = TheLevelSequence->GetMovieScene()->FindTrack<UMovieScene3DTransformTrack>(Bind.GetObjectGuid());
			for (UMovieSceneSection* TheSections : TransformTrack->GetAllSections())
			{
				UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(TheSections);
				if (TransformSection)
				{
					FMovieSceneChannelProxy& SectionChannelProxy = TransformSection->GetChannelProxy();
					TMovieSceneChannelHandle<FMovieSceneDoubleChannel> DoubleChannels[] = {SectionChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>("Rotation.X"), SectionChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>("Rotation.Y"), SectionChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>("Rotation.Z")};
					FMovieSceneDoubleChannel* ChannelX = DoubleChannels[0].Get();
					FMovieSceneDoubleChannel* ChannelY = DoubleChannels[1].Get();
					FMovieSceneDoubleChannel* ChannelZ = DoubleChannels[2].Get();

					for (auto Index_L = L_Start; Index_L <= L_End; ++Index_L)
					{
						FRotator CameraRot;
						ChannelX->Evaluate(Index_L, CameraRot.Roll);
						ChannelY->Evaluate(Index_L, CameraRot.Pitch);
						ChannelZ->Evaluate(Index_L, CameraRot.Yaw);
						CameraRot.Yaw -= 40;
						MainLightRot.Add(CameraRot);
					}
				}
			}
		}
	}
	if (!MainLightRot.IsEmpty())
	{
		/// 辅助光源
		DirectionalLight1->SetActorRotation(MainLightRot.HeapTop());
		FRotator Rot_Light = MainLightRot.HeapTop();
		Rot_Light.Yaw += 80;
		DirectionalLight2->SetActorRotation(Rot_Light);
		/// 添加辅助灯光旋转bind
		const FGuid L_GUID = TheLevelSequence->GetMovieScene()->AddPossessable(DirectionalLight1->GetActorLabel(), DirectionalLight1->GetClass());
		TheLevelSequence->BindPossessableObject(L_GUID, *DirectionalLight1, TheSequenceWorld);
		//-----------------------
		UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack_Light = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(L_GUID);
		UMovieSceneSpawnSection* L_MovieSceneSpawnSection_Light = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack_Light->CreateNewSection());
		L_MovieSceneSpawnTrack_Light->AddSection(*L_MovieSceneSpawnSection_Light);
		L_MovieSceneSpawnSection_Light->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
		UMovieScene3DTransformTrack* L_MovieSceneTranformTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieScene3DTransformTrack>(L_GUID);

		if (UMovieScene3DTransformSection* L_MovieScene3DTransformSection = Cast<UMovieScene3DTransformSection>(L_MovieSceneTranformTrack->CreateNewSection()); L_MovieScene3DTransformSection)
		{
			L_MovieSceneTranformTrack->AddSection(*L_MovieScene3DTransformSection);
			L_MovieScene3DTransformSection->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
			FMovieSceneChannelProxy& SectionChannelProxy = L_MovieScene3DTransformSection->GetChannelProxy();
			TMovieSceneChannelHandle<FMovieSceneDoubleChannel> DoubleChannels[] = {SectionChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>("Rotation.X"), SectionChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>("Rotation.Y"), SectionChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>("Rotation.Z")};
			FMovieSceneDoubleChannel* ChannelX = DoubleChannels[0].Get();
			FMovieSceneDoubleChannel* ChannelY = DoubleChannels[1].Get();
			FMovieSceneDoubleChannel* ChannelZ = DoubleChannels[2].Get();
			TArray<FFrameNumber> Times{};
			TArray<FMovieSceneDoubleValue> ValuesX{};
			TArray<FMovieSceneDoubleValue> ValuesY{};
			TArray<FMovieSceneDoubleValue> ValuesZ{};
			for (auto LT = L_Start; LT <= L_End; ++LT)
			{
				Times.Add(LT);
				ValuesX.Add(FMovieSceneDoubleValue{MainLightRot[(LT - L_Start).Value].Roll});
				ValuesY.Add(FMovieSceneDoubleValue{MainLightRot[(LT - L_Start).Value].Pitch});
				ValuesZ.Add(FMovieSceneDoubleValue{MainLightRot[(LT - L_Start).Value].Yaw});
			}
			ChannelX->AddKeys(Times, ValuesX);
			ChannelY->AddKeys(Times, ValuesY);
			ChannelZ->AddKeys(Times, ValuesZ);
		}
	}
}

UAssetImportTask* UDoodleAutoAnimationCommandlet::CreateGeometryImportTask(const FString& InFbxPath) const
{
	UAssetImportTask* Task = NewObject<UAssetImportTask>();
	Task->AddToRoot();
	Task->bAutomated = true;
	//L_Data->GroupName = TEXT("doodle import");
	Task->Filename = InFbxPath;
	FString AbcImportPath = FPaths::Combine(ImportPath, TEXT("Abc"));
	Task->DestinationPath = AbcImportPath;
	Task->bReplaceExisting = true;
	Task->bSave = true;
	//------------------------
	UDoodleAbcImportFactory* k_abc_f = NewObject<UDoodleAbcImportFactory>(Task);
	Task->Factory = k_abc_f;
	UDoodleAbcImportSettings* k_abc_stting = NewObject<UDoodleAbcImportSettings>(Task);
	k_abc_f->ImportSettings = k_abc_stting;
	//--------------------
	k_abc_stting->ImportType = EDoodleAlembicImportType::GeometryCache;
	k_abc_stting->ConversionSettings.bFlipV = true;
	k_abc_stting->ConversionSettings.Scale.X = 1.0;
	k_abc_stting->ConversionSettings.Scale.Y = -1.0;
	k_abc_stting->ConversionSettings.Scale.Z = 1.0;
	k_abc_stting->ConversionSettings.Rotation.X = 90.0;
	k_abc_stting->ConversionSettings.Rotation.Y = 0.0;
	k_abc_stting->ConversionSettings.Rotation.Z = 0.0;
	//--------------------------
	k_abc_stting->GeometryCacheSettings.bFlattenTracks = true;
	k_abc_stting->SamplingSettings.bSkipEmpty = true; //
	k_abc_stting->SamplingSettings.FrameStart = L_Start.Value; //
	k_abc_stting->SamplingSettings.FrameEnd = L_End.Value; // 
	k_abc_stting->SamplingSettings.FrameSteps = 1; //
	//------------
	k_abc_f->SetAssetImportTask(Task);
	return Task;
}

UAssetImportTask* UDoodleAutoAnimationCommandlet::CreateCharacterImportTask(const FString& InFbxPath) const
{
	UFbxFactory* K_FBX_F = NewObject<UFbxFactory>(UFbxFactory::StaticClass());
	K_FBX_F->ImportUI = NewObject<UFbxImportUI>(K_FBX_F);
	K_FBX_F->ImportUI->MeshTypeToImport = FBXIT_SkeletalMesh;
	K_FBX_F->ImportUI->OriginalImportType = FBXIT_SkeletalMesh;
	K_FBX_F->ImportUI->bImportAsSkeletal = true;
	K_FBX_F->ImportUI->bCreatePhysicsAsset = true;
	K_FBX_F->ImportUI->bImportMesh = true;
	K_FBX_F->ImportUI->bImportAnimations = true;
	K_FBX_F->ImportUI->bImportRigidMesh = true;
	K_FBX_F->ImportUI->bImportMaterials = false;
	K_FBX_F->ImportUI->bImportTextures = false;
	K_FBX_F->ImportUI->bResetToFbxOnMaterialConflict = false;
	//----------------------
	K_FBX_F->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
	K_FBX_F->ImportUI->SkeletalMeshImportData->bUseT0AsRefPose = false;
	K_FBX_F->ImportUI->bAutomatedImportShouldDetectType = false;
	K_FBX_F->ImportUI->AnimSequenceImportData->AnimationLength = FBXALIT_ExportedTime;
	K_FBX_F->ImportUI->AnimSequenceImportData->bImportBoneTracks = true;
	K_FBX_F->ImportUI->bAllowContentTypeImport = true;
	K_FBX_F->ImportUI->TextureImportData->MaterialSearchLocation = EMaterialSearchLocation::UnderRoot;
	//-----------------
	UAssetImportTask* Task = NewObject<UAssetImportTask>();
	Task->AddToRoot();
	Task->bAutomated = true;
	Task->bReplaceExisting = true;
	Task->DestinationPath = ImportPath;
	Task->bSave = true;
	Task->Options = K_FBX_F->ImportUI;
	Task->Filename = InFbxPath;
	Task->Factory = K_FBX_F;
	K_FBX_F->SetAssetImportTask(Task);
	return Task;
}

void UDoodleAutoAnimationCommandlet::OnBuildSequence()
{
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	TArray<UGeometryCache*> GeometryCaches;
	TArray<UAssetImportTask*> ImportTasks;
	TArray<UAssetImportTask*> ImportTasksAbc;
	for (const auto& [Type, Path] : ImportFiles)
	{
		switch (Type)
		{
		case EImportFilesType2::Camera:
			ImportCamera(Path);
			break;
		case EImportFilesType2::Geometry:
			ImportTasksAbc.Add(CreateGeometryImportTask(Path));
			break;
		case EImportFilesType2::Character:
			ImportTasks.Add(CreateCharacterImportTask(Path));
			break;
		}
	}


	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.ImportAssetTasks(ImportTasks);
	for (UAssetImportTask* Task : ImportTasks)
	{
		if (Task->IsAsyncImportComplete())
		{
			if (TArray<UObject*> ImportedObjs = Task->GetObjects(); !ImportedObjs.IsEmpty())
			{
				if (UObject* ImportedObject = ImportedObjs.Top(); ImportedObject->GetClass()->IsChildOf(USkeletalMesh::StaticClass()))
				{
					USkeletalMesh* TmpSkeletalMesh = Cast<USkeletalMesh>(ImportedObject);
					UAnimSequence* AnimSeq = nullptr;
					TArray<FAssetData> OutAssetData;
					IAssetRegistry::Get()->GetAssetsByPath(FName(*ImportPath), OutAssetData, false);
					for (const FAssetData& Asset : OutAssetData)
					{
						EditorAssetSubsystem->SaveLoadedAsset(Asset.GetAsset());
						if (UAnimSequence* Anim = Cast<UAnimSequence>(Asset.GetAsset()); Anim && Anim->GetSkeleton() == TmpSkeletalMesh->GetSkeleton())
						{
							AnimSeq = Anim;
							break;
						}
					}
					//------------
					if (AnimSeq)
					{
						AnimSeq->BoneCompressionSettings = LoadObject<UAnimBoneCompressionSettings>(AnimSeq, TEXT("/Engine/Animation/DefaultRecorderBoneCompression.DefaultRecorderBoneCompression"));
						if (AnimSeq->IsDataModelValid())
						{
							AnimSeq->CompressCommandletVersion = 0;
							AnimSeq->ClearAllCachedCookedPlatformData();
							AnimSeq->CacheDerivedDataForCurrentPlatform();
						}
						//------------------
						ASkeletalMeshActor* L_Actor = TheSequenceWorld->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
						L_Actor->SetActorLabel(TmpSkeletalMesh->GetName());
						L_Actor->GetSkeletalMeshComponent()->SetSkeletalMesh(TmpSkeletalMesh);
						L_Actor->GetSkeletalMeshComponent()->SetLightingChannels(false, true, false);
						L_Actor->GetSkeletalMeshComponent()->SetReceivesDecals(false);
						//---------------------
						const FGuid L_GUID = TheLevelSequence->GetMovieScene()->AddPossessable(L_Actor->GetActorLabel(), L_Actor->GetClass());
						TheLevelSequence->BindPossessableObject(L_GUID, *L_Actor, TheSequenceWorld);
						//-----------------------
						UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(L_GUID);
						UMovieSceneSpawnSection* L_MovieSceneSpawnSection = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
						L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);
						L_MovieSceneSpawnSection->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
						UMovieSceneSkeletalAnimationTrack* L_MovieSceneSkeletalAnim = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSkeletalAnimationTrack>(L_GUID);
						UMovieSceneSection* AnimSection = L_MovieSceneSkeletalAnim->AddNewAnimationOnRow(L_Start, AnimSeq, -1);
						AnimSection->SetPreRollFrames(50);
						AnimSection->Modify();
						//--------------------------Clone------------------------
						ASkeletalMeshActor* L_Actor2 = TheSequenceWorld->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
						L_Actor2->SetActorLabel(TmpSkeletalMesh->GetName() + TEXT("_SH"));
						L_Actor2->GetSkeletalMeshComponent()->SetSkeletalMesh(TmpSkeletalMesh);
						L_Actor2->GetSkeletalMeshComponent()->SetLightingChannels(true, false, false);
						L_Actor2->GetSkeletalMeshComponent()->SetVisibility(false);
						L_Actor2->GetSkeletalMeshComponent()->SetCastHiddenShadow(true);
						L_Actor2->GetSkeletalMeshComponent()->SetReceivesDecals(false);
						const FGuid L_GUID2 = TheLevelSequence->GetMovieScene()->AddPossessable(L_Actor2->GetActorLabel(), L_Actor2->GetClass());
						TheLevelSequence->BindPossessableObject(L_GUID2, *L_Actor2, TheSequenceWorld);
						UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack2 = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(L_GUID2);
						UMovieSceneSpawnSection* L_MovieSceneSpawnSection2 = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack2->CreateNewSection());
						L_MovieSceneSpawnTrack2->AddSection(*L_MovieSceneSpawnSection2);
						L_MovieSceneSpawnSection2->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
						UMovieSceneSkeletalAnimationTrack* L_MovieSceneSkeletalAnim2 = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSkeletalAnimationTrack>(L_GUID2);
						UMovieSceneSection* AnimSection2 = L_MovieSceneSkeletalAnim2->AddNewAnimationOnRow(L_Start, AnimSeq, -1);
						AnimSection2->SetPreRollFrames(50);
						AnimSection2->Modify();
						//----------------------
						TheLevelSequence->GetMovieScene()->Modify();
						TheLevelSequence->Modify();
						TheSequenceWorld->Modify();
					}
				}
			}
		}
		Task->RemoveFromRoot();
	}
	AssetTools.ImportAssetTasks(ImportTasksAbc);
	for (UAssetImportTask* Task : ImportTasksAbc)
	{
		if (Task->IsAsyncImportComplete())
		{
			TArray<UObject*> ImportedObjs = Task->GetObjects();
			EditorAssetSubsystem->SaveLoadedAssets(ImportedObjs);
			if (!ImportedObjs.IsEmpty())
			{
				UObject* ImportedObject = ImportedObjs.Top();;
				if (ImportedObject->GetClass()->IsChildOf(UGeometryCache::StaticClass()))
				{
					UGeometryCache* TempGeometryCache = Cast<UGeometryCache>(ImportedObject);
					//----------
					AGeometryCacheActor* L_Actor = TheSequenceWorld->SpawnActor<AGeometryCacheActor>(FVector::ZeroVector, FRotator::ZeroRotator);
					L_Actor->SetActorLabel(TempGeometryCache->GetName());
					L_Actor->GetGeometryCacheComponent()->SetGeometryCache(TempGeometryCache);
					L_Actor->GetGeometryCacheComponent()->SetLightingChannels(false, true, false);
					//---------------------------------
					const FGuid L_GUID = TheLevelSequence->GetMovieScene()->AddPossessable(L_Actor->GetActorLabel(), L_Actor->GetClass());
					TheLevelSequence->BindPossessableObject(L_GUID, *L_Actor, TheSequenceWorld);
					//-----------------------------------
					UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(L_GUID);
					UMovieSceneSpawnSection* L_MovieSceneSpawnSection = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
					L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);
					//------------------------------
					UMovieSceneGeometryCacheTrack* L_MovieSceneGeoTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneGeometryCacheTrack>(L_GUID);
					UMovieSceneSection* AnimSection = L_MovieSceneGeoTrack->AddNewAnimation(L_Start, L_Actor->GetGeometryCacheComponent());
					AnimSection->SetPreRollFrames(50);
					L_Actor->Modify();
					//---------------------------Clone----------------------------
					AGeometryCacheActor* L_Actor2 = TheSequenceWorld->SpawnActor<AGeometryCacheActor>(FVector::ZeroVector, FRotator::ZeroRotator);
					L_Actor2->SetActorLabel(TempGeometryCache->GetName() + TEXT("_SH"));
					L_Actor2->GetGeometryCacheComponent()->SetGeometryCache(TempGeometryCache);
					L_Actor2->GetGeometryCacheComponent()->SetLightingChannels(true, false, false);
					L_Actor2->GetGeometryCacheComponent()->SetVisibility(false);
					L_Actor2->GetGeometryCacheComponent()->SetCastHiddenShadow(true);
					const FGuid L_GUID2 = TheLevelSequence->GetMovieScene()->AddPossessable(L_Actor2->GetActorLabel(), L_Actor2->GetClass());
					TheLevelSequence->BindPossessableObject(L_GUID2, *L_Actor2, TheSequenceWorld);
					UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack2 = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(L_GUID2);
					UMovieSceneSpawnSection* L_MovieSceneSpawnSection2 = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack2->CreateNewSection());
					L_MovieSceneSpawnTrack2->AddSection(*L_MovieSceneSpawnSection2);
					UMovieSceneGeometryCacheTrack* L_MovieSceneGeoTrack2 = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneGeometryCacheTrack>(L_GUID2);
					UMovieSceneSection* AnimSection2 = L_MovieSceneGeoTrack2->AddNewAnimation(L_Start, L_Actor2->GetGeometryCacheComponent());
					AnimSection2->SetPreRollFrames(50);
					L_Actor2->Modify();
					//------------------------
					TheLevelSequence->GetMovieScene()->Modify();
					TheLevelSequence->Modify();
					TheSequenceWorld->Modify();
				}
			}
		}
		Task->RemoveFromRoot();
	}
	EditorAssetSubsystem->SaveLoadedAssets({TheLevelSequence, TheSequenceWorld});
}

void UDoodleAutoAnimationCommandlet::OnBuildCheckCharacter()
{
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	TArray<UAssetImportTask*> ImportTasks;
	for (const auto& [Type, Path] : ImportFiles)
	{
		switch (Type)
		{
		case EImportFilesType2::Camera:
			ImportCamera(Path);
			break;
		case EImportFilesType2::Geometry:
			break;
		case EImportFilesType2::Character:
			ImportTasks.Add(CreateCharacterImportTask(Path));
			break;
		}
	}
	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.ImportAssetTasks(ImportTasks);


	USkeletalMesh* TmpSkeletalMesh{};
	for (UAssetImportTask* Task : ImportTasks)
	{
		if (TArray<UObject*> ImportedObjs = Task->GetObjects(); !ImportedObjs.IsEmpty())
		{
			if (UObject* ImportedObject = ImportedObjs.Top(); ImportedObject->GetClass()->IsChildOf(USkeletalMesh::StaticClass()))
			{
				TmpSkeletalMesh = Cast<USkeletalMesh>(ImportedObject);
				UAnimSequence* AnimSeq = nullptr;
				TArray<FAssetData> OutAssetData;
				IAssetRegistry::Get()->GetAssetsByPath(FName(*ImportPath), OutAssetData, false);
				for (const FAssetData& Asset : OutAssetData)
				{
					EditorAssetSubsystem->SaveLoadedAsset(Asset.GetAsset());
					if (UAnimSequence* Anim = Cast<UAnimSequence>(Asset.GetAsset()); Anim && Anim->GetSkeleton() == TmpSkeletalMesh->GetSkeleton())
					{
						AnimSeq = Anim;
						break;
					}
				}
				//------------
				if (AnimSeq)
				{
					AnimSeq->BoneCompressionSettings = LoadObject<UAnimBoneCompressionSettings>(AnimSeq, TEXT("/Engine/Animation/DefaultRecorderBoneCompression.DefaultRecorderBoneCompression"));
					if (AnimSeq->IsDataModelValid())
					{
						AnimSeq->CompressCommandletVersion = 0;
						AnimSeq->ClearAllCachedCookedPlatformData();
						AnimSeq->CacheDerivedDataForCurrentPlatform();
					}
					//------------------
					ASkeletalMeshActor* L_Actor = TheSequenceWorld->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator{0, 180, 0});
					L_Actor->SetActorLabel(TmpSkeletalMesh->GetName());
					L_Actor->GetSkeletalMeshComponent()->SetSkeletalMesh(TmpSkeletalMesh);
					//---------------------
					const FGuid L_GUID = TheLevelSequence->GetMovieScene()->AddPossessable(L_Actor->GetActorLabel(), L_Actor->GetClass());
					TheLevelSequence->BindPossessableObject(L_GUID, *L_Actor, TheSequenceWorld);
					//-----------------------
					UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(L_GUID);
					UMovieSceneSpawnSection* L_MovieSceneSpawnSection = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
					L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);
					L_MovieSceneSpawnSection->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
					UMovieSceneSkeletalAnimationTrack* L_MovieSceneSkeletalAnim = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSkeletalAnimationTrack>(L_GUID);
					UMovieSceneSection* AnimSection = L_MovieSceneSkeletalAnim->AddNewAnimationOnRow(L_Start, AnimSeq, -1);
					AnimSection->SetPreRollFrames(50);
					AnimSection->Modify();
					//----------------------
					TheLevelSequence->GetMovieScene()->Modify();
					TheLevelSequence->Modify();
					TheSequenceWorld->Modify();
				}
			}
		}
	}


	UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();

	AActor* TempActor = EditorActorSubsystem->SpawnActorFromClass(ACineCameraActor::StaticClass(), FVector::ZAxisVector, FRotator::ZeroRotator, false);
	ACineCameraActor* CameraActor = CastChecked<ACineCameraActor>(TheLevelSequence->MakeSpawnableTemplateFromInstance(*TempActor, TempActor->GetFName()));
	CameraActor->GetCineCameraComponent()->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
	FGuid CameraGuid = TheLevelSequence->GetMovieScene()->AddSpawnable(CameraActor->GetName(), *CameraActor);
	//---------------
	UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(CameraGuid);
	UMovieSceneSpawnSection* L_MovieSceneSpawnSection = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
	L_MovieSceneSpawnSection->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
	L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);
	//------------------------
	MovieSceneToolHelpers::CreateCameraCutSectionForCamera(TheLevelSequence->GetMovieScene(), CameraGuid, L_Start);
	UMovieSceneTrack* CameraCutTrack = TheLevelSequence->GetMovieScene()->GetCameraCutTrack();
	UMovieSceneCameraCutSection* CutSection;
	if (CameraCutTrack->GetAllSections().Num() <= 0)
	{
		CutSection = Cast<UMovieSceneCameraCutSection>(CameraCutTrack->CreateNewSection());
		CameraCutTrack->AddSection(*CutSection);
	}
	else
	{
		CutSection = CastChecked<UMovieSceneCameraCutSection>(CameraCutTrack->GetAllSections().Top());
	}
	//--------------------
	CutSection->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
	CutSection->SetCameraGuid(CameraGuid);
	//FMovieSceneObjectBindingID BindingID = CutSection->GetCameraBindingID();
	//------------------
	//FString CameraLabel = CameraActor->GetActorNameOrLabel();
	//TMap<FGuid, FString> L_Map{};
	//L_Map.Add(BindingID.GetGuid(), CameraLabel);

	TempActor->Destroy();
	// 添加相机位置

	if (TmpSkeletalMesh)
	{
		FBoxSphereBounds LBox = TmpSkeletalMesh->GetBounds();
		const FVector Position = LBox.GetBox().GetCenter();
		FViewportCameraTransform LViewportCameraTransform{};
		float Radius = FMath::Max<FVector::FReal>(LBox.GetBox().GetExtent().Size(), 10.0);
		if (const float AspectRatio = CameraActor->GetCameraComponent()->AspectRatio; AspectRatio > 1.0f)
		{
			Radius *= AspectRatio;
		}

		const float HalfFOVRadians = FMath::DegreesToRadians(Cast<UCineCameraComponent>(CameraActor->GetCameraComponent())->GetHorizontalFieldOfView() / 2.0f);
		const float DistanceFromSphere = Radius / FMath::Tan(HalfFOVRadians);
		FVector CameraOffsetVector = LViewportCameraTransform.GetRotation().Vector() * -DistanceFromSphere;

		LViewportCameraTransform.SetLocation(Position + CameraOffsetVector);
		LViewportCameraTransform.SetLookAt(Position);
		FTransform LTransform{LViewportCameraTransform.ComputeOrbitMatrix().Inverse()};
		CameraActor->SetActorTransform(LTransform);
	}

	EditorAssetSubsystem->SaveLoadedAssets({TheLevelSequence, TheSequenceWorld});
}

void UDoodleAutoAnimationCommandlet::OnBuildCheckScene()
{
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	// 先添加相机剪裁
	UMovieSceneTrack* CameraCutTrack = TheLevelSequence->GetMovieScene()->GetCameraCutTrack();
	if (!CameraCutTrack) CameraCutTrack = TheLevelSequence->GetMovieScene()->AddCameraCutTrack(UMovieSceneCameraCutTrack::StaticClass());
	int LSize{};
	for (TActorIterator<ACameraActor> CameraItr{TheRenderWorld}; CameraItr; ++CameraItr)
	{
		LSize = ++LSize;
		UE_LOG(LogTemp, Log, TEXT("ca, name %s"), *CameraItr->GetActorLabel())
	}
	FFrameNumberRange LRange = TheLevelSequence->GetMovieScene()->GetPlaybackRange();
	FFrameNumber LBegin = LRange.GetLowerBoundValue();
	FFrameNumber LFrameSize = LRange.Size<FFrameNumber>();

	int LSize2{};
	for (TActorIterator<ACameraActor> CameraItr{TheRenderWorld}; CameraItr; ++CameraItr)
	{
		FGuid CameraGuid = TheLevelSequence->GetMovieScene()->AddPossessable(CameraItr->GetActorLabel(), CameraItr->GetClass());
		TheLevelSequence->BindPossessableObject(CameraGuid, **CameraItr, TheRenderWorld);
		//---------------
		UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(CameraGuid);
		UMovieSceneSpawnSection* L_MovieSceneSpawnSection = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
		L_MovieSceneSpawnSection->SetRange(FFrameNumberRange{LBegin + (LFrameSize / LSize) * LSize2, LBegin + (LFrameSize / LSize) * (LSize2 + 1)});
		L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);

		//创建相机剪裁切换
		UMovieSceneCameraCutSection* NewSection = Cast<UMovieSceneCameraCutSection>(CameraCutTrack->CreateNewSection());
		NewSection->SetRange(FFrameNumberRange{LBegin + (LFrameSize / LSize) * LSize2, LBegin + (LFrameSize / LSize) * (LSize2 + 1)});
		NewSection->SetCameraGuid(CameraGuid);
		CameraCutTrack->AddSection(*NewSection);

		++LSize2;
	}

	EditorAssetSubsystem->SaveLoadedAssets({TheLevelSequence, TheSequenceWorld});
}

void UDoodleAutoAnimationCommandlet::OnSaveReanderConfig()
{
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();

	const FString ConfigName = FPaths::GetBaseFilename(MoviePipelineConfigPath);
	//------------
	UMoviePipelinePrimaryConfig* Config = LoadObject<UMoviePipelinePrimaryConfig>(nullptr, *MoviePipelineConfigPath);
	if (!Config)
	{
		UPackage* Package = CreatePackage(*MoviePipelineConfigPath);
		Config = NewObject<UMoviePipelinePrimaryConfig>(Package, *ConfigName, RF_Public | RF_Standalone | RF_Transactional);
		IAssetRegistry::GetChecked().AssetCreated(Config);
		Package->Modify();
	}

	//----------------------
	UMoviePipelineOutputSetting* OutputSetting = Cast<UMoviePipelineOutputSetting>(Config->FindOrAddSettingByClass(UMoviePipelineOutputSetting::StaticClass()));
	OutputSetting->OutputDirectory.Path = DestinationPath;
	if (UMoviePipelineImageSequenceOutput_JPG* FormatSetting = Config->FindSetting<UMoviePipelineImageSequenceOutput_JPG>())
	{
		Config->RemoveSetting(FormatSetting);
	}
	Config->FindOrAddSettingByClass(UMoviePipelineImageSequenceOutput_PNG::StaticClass());
	Config->FindOrAddSettingByClass(UMoviePipelineDeferredPassBase::StaticClass());

	// 设置抗拒齿方法
	UMoviePipelineAntiAliasingSetting* AntiAliasing = Cast<UMoviePipelineAntiAliasingSetting>(Config->FindOrAddSettingByClass(UMoviePipelineAntiAliasingSetting::StaticClass()));
	if (AntiAliasing)
	{
		AntiAliasing->SpatialSampleCount = 1;
		AntiAliasing->TemporalSampleCount = 1;
		AntiAliasing->bOverrideAntiAliasing = true;
		AntiAliasing->AntiAliasingMethod = EAntiAliasingMethod::AAM_TSR;
		AntiAliasing->bRenderWarmUpFrames = true;
		AntiAliasing->EngineWarmUpCount = 64;
	}
	if (UMoviePipelineGameOverrideSetting* GameOver = Cast<UMoviePipelineGameOverrideSetting>(Config->FindOrAddSettingByClass(UMoviePipelineGameOverrideSetting::StaticClass())); GameOver)
	{
		GameOver->bCinematicQualitySettings = false;
	}
	if (UMoviePipelineConsoleVariableSetting* ConsoleVar = Cast<UMoviePipelineConsoleVariableSetting>(Config->FindOrAddSettingByClass(UMoviePipelineConsoleVariableSetting::StaticClass())))
	{
		Config->RemoveSetting(ConsoleVar);
	}

	//-------------------------Save
	Config->SetFlags(RF_Public | RF_Transactional | RF_Standalone);
	Config->MarkPackageDirty();
	//--------------------------
	FAssetRegistryModule::AssetCreated(Config);
	EditorAssetSubsystem->SaveLoadedAsset(Config);
}

void UDoodleAutoAnimationCommandlet::FixMaterialProperty()
{
	FARFilter LFilter{};
	LFilter.bIncludeOnlyOnDiskAssets = false;
	LFilter.bRecursivePaths = true;
	LFilter.bRecursiveClasses = true;
	LFilter.PackagePaths = TArray<FName>{FName{TEXT("/Game/Character/")}, FName{TEXT("/Game/Prop/")}};
	LFilter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());

	TArray<UObject*> L_Save{};

	IAssetRegistry::Get()->EnumerateAssets(LFilter, [&](const FAssetData& InAss) -> bool
	{
		if (FPaths::IsUnderDirectory(InAss.PackagePath.ToString(), TEXT("/Game/Character/")) || FPaths::IsUnderDirectory(InAss.PackagePath.ToString(), TEXT("/Game/Prop/")))
		{
			if (UMaterial* L_Mat = Cast<UMaterial>(InAss.GetAsset()))
			{
				bool L_bHasProperty{true};
				L_Mat->SetMaterialUsage(L_bHasProperty, EMaterialUsage::MATUSAGE_GeometryCache);
				L_Mat->SetMaterialUsage(L_bHasProperty, EMaterialUsage::MATUSAGE_SkeletalMesh);
				L_Mat->SetMaterialUsage(L_bHasProperty, EMaterialUsage::MATUSAGE_MorphTargets);
				L_Save.Add(L_Mat);
			};
		}
		return true;
	});
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	EditorAssetSubsystem->SaveLoadedAssets(L_Save);
}


//"D:\\Program Files\\Epic Games\\UE_5.2\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" "D:/Users/Administrator/Documents/Unreal Projects/MyProject/MyProject.uproject" -skipcompile -run=DoodleAutoAnimation  -Params=D:/test_files/test_ue_auto_main/out.json
//"D:\\Program Files\\Epic Games\\UE_5.2\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" "D:/Users/Administrator/Documents/Unreal Projects/MyProject/MyProject.uproject" -skipcompile -run=DoodleAutoAnimation  -Check=E:/Doodle/build/test_ue_check.json
//"D:\\Program Files\\Epic Games\\UE_5.2\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" "D:/Users/Administrator/Documents/Unreal Projects/MyProject/MyProject.uproject" -skipcompile -run=DoodleAutoAnimation  -Check=E:/Doodle/build/test_ue_check_secen.json
//UnrealEditor-Cmd.exe D:\\Users\\Administrator\\Documents\\Unreal Projects\\MyProject\\MyProject.uproject -skipcompile -run=DoodleAutoAnimation  -Params=E:/AnimationImport/DBXY_EP360_SC001_AN/out.json
