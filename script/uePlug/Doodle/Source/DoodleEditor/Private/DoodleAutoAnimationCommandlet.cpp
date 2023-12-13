// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleAutoAnimationCommandlet.h"

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

UDoodleAutoAnimationCommandlet::UDoodleAutoAnimationCommandlet()
{
	LogToConsole = true;
}

int32 UDoodleAutoAnimationCommandlet::Main(const FString& Params)
{
	FString FilePath = TEXT("");
	TArray<FString> CmdLineTokens;
	TArray<FString> CmdLineSwitches;
	//------------------
    TMap<FString, FString> ParamsMap;
    ParseCommandLine(*Params, CmdLineTokens, CmdLineSwitches, ParamsMap);
	//--------------
	const FString& Key = TEXT("Params");
	if (ParamsMap.Contains(Key))
	{
		FilePath = ParamsMap[Key];
		//UE_LOG(LogTemp,Log, TEXT("%s"), *FilePath);
	}
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No params field in cmd arguments"));
        return -1;
    }
	//--------------------
    FString JsonString;
    if (FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);

        FJsonSerializer::Deserialize(JsonReader, JsonObject);
    }
    //--------------------
    OriginalMapPath = FName(JsonObject->GetStringField(TEXT("original_map")));
    RenderMapPath = FName(JsonObject->GetStringField(TEXT("render_map")));
    CreateMapPath = FName(JsonObject->GetStringField(TEXT("create_map")));
    //---------------------------
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    if (EditorAssetSubsystem->DoesAssetExist(RenderMapPath.ToString()))
    {
        EditorAssetSubsystem->DeleteAsset(RenderMapPath.ToString());
    }
    OnCreateSequence();
    //--------------
    ImportPath = JsonObject->GetStringField(TEXT("import_dir"));
    OnCreateSequenceWorld();
    EditorAssetSubsystem->DuplicateAsset(OriginalMapPath.ToString(), RenderMapPath.ToString());
    OnBuildSequence();
    //---------------------
    UWorld* RenderLevel = LoadObject<UWorld>(nullptr, *RenderMapPath.ToString());
    ULevelStreaming* TempStreamingLevel = NewObject<ULevelStreaming>(RenderLevel, ULevelStreamingDynamic::StaticClass(), NAME_None, RF_NoFlags, NULL);
    TempStreamingLevel->SetWorldAsset(TheSequenceWorld);
    RenderLevel->AddStreamingLevel(TempStreamingLevel);
    //---------------
    UMovieSceneLevelVisibilityTrack* NewTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneLevelVisibilityTrack>();
    UMovieSceneLevelVisibilitySection* NewSection = CastChecked<UMovieSceneLevelVisibilitySection>(NewTrack->CreateNewSection());
    TRange<FFrameNumber> SectionRange = TheLevelSequence->GetMovieScene()->GetPlaybackRange();
    NewSection->SetRange(SectionRange);
    NewTrack->AddSection(*NewSection);
    //-----------------
    TArray<FName> LevelNames;
    for (ULevelStreaming* StreamingLevel : RenderLevel->GetStreamingLevels())
    {
        if (StreamingLevel && !LevelNames.Contains(StreamingLevel->GetWorldAssetPackageFName()))
        {
            LevelNames.Add(StreamingLevel->GetWorldAssetPackageFName());
        }
    }
    LevelNames.Add(RenderMapPath);
    NewSection->SetLevelNames(LevelNames);
    //NewSection->SetVisibility(ELevelVisibility::Visible);
    EditorAssetSubsystem->SaveLoadedAssets({ TheLevelSequence,RenderLevel });
    //-----------------
    OnSaveReanderConfig();
    return 0;
}

void UDoodleAutoAnimationCommandlet::OnCreateSequence()
{
    SequencePath = JsonObject->GetStringField(TEXT("level_sequence"));
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
    FString AssetName = FPaths::GetBaseFilename(SequencePath);
    TheLevelSequence = LoadObject<ULevelSequence>(nullptr,*(SequencePath));
    if (!TheLevelSequence)
    {
        UPackage* Package = CreatePackage(*SequencePath);
        TheLevelSequence = NewObject<ULevelSequence>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
        TheLevelSequence->Initialize();
        IAssetRegistry::GetChecked().AssetCreated(TheLevelSequence);
        Package->Modify();
    }
    //------------
    const FFrameRate L_Rate{ 25, 1 };
    FFrameNumber Offset{ 0 };
    L_Start = FFrameNumber(JsonObject->GetIntegerField(TEXT("begin_time")));
    L_End = FFrameNumber(JsonObject->GetIntegerField(TEXT("end_time")));
    //--------------------------
    TheLevelSequence->GetMovieScene()->SetDisplayRate(L_Rate);
    TheLevelSequence->GetMovieScene()->SetTickResolutionDirectly(L_Rate);
    //--------------------
    TheLevelSequence->GetMovieScene()->SetWorkingRange((L_Start - 30 - Offset) / L_Rate, (L_End + 30) / L_Rate);
    TheLevelSequence->GetMovieScene()->SetViewRange((L_Start - 30 - Offset) / L_Rate, (L_End + 30) / L_Rate);
    TheLevelSequence->GetMovieScene()->SetPlaybackRange(TRange<FFrameNumber>{L_Start - Offset, L_End}, true);
    TheLevelSequence->GetMovieScene()->Modify();
    //----------------
    TArray<FMovieSceneBinding> Bindings = TheLevelSequence->GetMovieScene()->GetBindings();
    for (FMovieSceneBinding Bind : Bindings)
    {
        if (!TheLevelSequence->GetMovieScene()->RemovePossessable(Bind.GetObjectGuid()))
        {
            TheLevelSequence->GetMovieScene()->RemoveSpawnable(Bind.GetObjectGuid());
        }
    }
    TArray<UMovieSceneTrack*> Tracks = TheLevelSequence->GetMovieScene()->GetTracks();
    for (UMovieSceneTrack* Track : Tracks)
    {
        TheLevelSequence->GetMovieScene()->RemoveTrack(*Track);
    }
}

void UDoodleAutoAnimationCommandlet::OnCreateSequenceWorld()
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    CreateMapPath = FName(JsonObject->GetStringField(TEXT("create_map")));
    TheSequenceWorld = LoadObject<UWorld>(nullptr, *CreateMapPath.ToString());
    if (!TheSequenceWorld)
    {
        UWorldFactory* Factory = NewObject<UWorldFactory>();
        UPackage* Pkg = CreatePackage(*CreateMapPath.ToString());
        Pkg->FullyLoad();
        Pkg->MarkPackageDirty();
        const FString PackagePath = FPackageName::GetLongPackagePath(CreateMapPath.ToString());
        FString BaseFileName = FPaths::GetBaseFilename(CreateMapPath.ToString());
        //---------------
        FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
        UObject* TempObject = AssetToolsModule.Get().CreateAsset(BaseFileName, PackagePath, UWorld::StaticClass(), Factory);
        TheSequenceWorld = Cast<UWorld>(TempObject);
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        AssetRegistryModule.Get().AssetCreated(TheSequenceWorld);
        //------------------------
        TheSequenceWorld->Modify();
        EditorAssetSubsystem->SaveLoadedAsset(TheSequenceWorld);
    }
    //--------------------
    ULevel* Level = TheSequenceWorld->GetCurrentLevel();
    for (int32 Index = 0; Index < Level->Actors.Num(); Index++)
    {
        AActor* Actor = Level->Actors[Index];
        if (Actor != nullptr && Actor->IsA<ASkeletalMeshActor>())
        {
            Actor->Destroy();
        }
    }
}

void UDoodleAutoAnimationCommandlet::OnBuildSequence()
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
    TArray<TSharedPtr<FJsonValue>> JsonFiles = JsonObject->GetArrayField(TEXT("files"));
    for (TSharedPtr<FJsonValue> JsonFile : JsonFiles)
    {
        TSharedPtr<FJsonObject> Obj = JsonFile->AsObject();
        const FString Path = Obj->GetStringField(TEXT("path"));
        if (FPaths::FileExists(Path))
        {
            const FString Type = Obj->GetStringField(TEXT("type"));
            if (Type == TEXT("cam"))
            {
                AActor* TempActor = EditorActorSubsystem->SpawnActorFromClass(ACineCameraActor::StaticClass(), FVector::ZAxisVector, FRotator::ZeroRotator, false);
                ACineCameraActor* CameraActor = CastChecked<ACineCameraActor>(TheLevelSequence->MakeSpawnableTemplateFromInstance(*TempActor, TempActor->GetFName()));
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
                MovieSceneToolHelpers::ReadyFBXForImport(Path, L_ImportFBXSettings, InOutParams);
                //---------------------
                UnFbx::FFbxImporter* FbxImporter = UnFbx::FFbxImporter::GetInstance();
                FbxImporter->ImportFromFile(*Path, FPaths::GetExtension(Path));
                MovieSceneToolHelpers::ImportFBXCameraToExisting(FbxImporter, TheLevelSequence, L_LevelSequencePlayer, BindingID.GetRelativeSequenceID(), L_Map, false, false);
                //---------------------
                bool bValid = MovieSceneToolHelpers::ImportFBXIfReady(GWorld, TheLevelSequence, L_LevelSequencePlayer, BindingID.GetRelativeSequenceID(), L_Map, L_ImportFBXSettings, InOutParams);
                //----------------
                TempActor->Destroy();
                L_LevelSequenceActor->Destroy();
                FbxImporter->ReleaseScene();
            }
            else
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
                Task->Filename = Path;
                Task->Factory = K_FBX_F;
                K_FBX_F->SetAssetImportTask(Task);
                ImportTasks.Add(Task);
            }
        }
    }
    IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
    AssetTools.Get().ImportAssetTasks(ImportTasks);
    for (UAssetImportTask* Task : ImportTasks)
    {
        if (Task->IsAsyncImportComplete())
        {
            TArray<UObject*> ImportedObjs = Task->GetObjects();
            if (ImportedObjs.Num() > 0)
            {
                UObject* ImportedObject = ImportedObjs.Top();
                USkeletalMesh* TmpSkeletalMesh = Cast<USkeletalMesh>(ImportedObject);
                if (TmpSkeletalMesh)
                {
                    UAnimSequence* AnimSeq = nullptr;
                    TArray<FAssetData> OutAssetData;
                    IAssetRegistry::Get()->GetAssetsByPath(FName(*ImportPath), OutAssetData, false);
                    for (FAssetData Asset : OutAssetData)
                    {
                        EditorAssetSubsystem->SaveLoadedAsset(Asset.GetAsset());
                        UAnimSequence* Anim = Cast<UAnimSequence>(Asset.GetAsset());
                        if (Anim && Anim->GetSkeleton() == TmpSkeletalMesh->GetSkeleton())
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
                            AnimSeq->ClearAllCachedCookedPlatformData();
                            AnimSeq->BeginCacheDerivedDataForCurrentPlatform();
                        }
                        //------------------
                        ASkeletalMeshActor* L_Actor = TheSequenceWorld->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
                        L_Actor->SetActorLabel(TmpSkeletalMesh->GetName());
                        L_Actor->GetSkeletalMeshComponent()->SetSkeletalMesh(TmpSkeletalMesh);
                        //---------------------
                        const FGuid L_GUID = TheLevelSequence->GetMovieScene()->AddPossessable(L_Actor->GetActorLabel(), L_Actor->GetClass());
                        TheLevelSequence->BindPossessableObject(L_GUID, *L_Actor,TheSequenceWorld);
                        TheLevelSequence->GetMovieScene()->Modify();
                        //-----------------------
                        UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(L_GUID);
                        UMovieSceneSpawnSection* L_MovieSceneSpawnSection = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
                        L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);
                        //---------------
                        L_MovieSceneSpawnSection->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
                        UMovieSceneSkeletalAnimationTrack* L_MovieSceneSkeletalAnim = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSkeletalAnimationTrack>(L_GUID);
                        int32_t StartTime = { 1000 };
                        UMovieSceneSection* AnimSection = L_MovieSceneSkeletalAnim->AddNewAnimationOnRow(StartTime, AnimSeq, -1);
                        //AnimSection->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
                        //AnimSection->SetPreRollFrames(50);
                        AnimSection->Modify();
                    }
                }
            }
        }
        Task->RemoveFromRoot();
    }
    EditorAssetSubsystem->SaveLoadedAssets({ TheLevelSequence,TheSequenceWorld });
}

void UDoodleAutoAnimationCommandlet::OnSaveReanderConfig()
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    DestinationPath = JsonObject->GetStringField(TEXT("out_file_dir"));
    //------------
    UMoviePipelinePrimaryConfig* Config = NewObject<UMoviePipelinePrimaryConfig>(GetTransientPackage());
    Config->SetFlags(RF_Transactional);
    UMoviePipelineOutputSetting* OutputSetting = Cast<UMoviePipelineOutputSetting>(Config->FindOrAddSettingByClass(UMoviePipelineOutputSetting::StaticClass()));
    OutputSetting->OutputDirectory.Path = DestinationPath;
    //----------------------
    UMoviePipelineImageSequenceOutput_JPG* FormatSetting = Config->FindSetting<UMoviePipelineImageSequenceOutput_JPG>();
    if (FormatSetting)
    {
        Config->RemoveSetting(FormatSetting);
    }
    Config->FindOrAddSettingByClass(UMoviePipelineImageSequenceOutput_PNG::StaticClass());
    Config->FindOrAddSettingByClass(UMoviePipelineDeferredPassBase::StaticClass());
    //-------------------------Save
    MoviePipelineConfigPath = JsonObject->GetStringField(TEXT("movie_pipeline_config"));
    FString ConfigName = FPaths::GetBaseFilename(MoviePipelineConfigPath);
    //-----------------
    UPackage* NewPackage = CreatePackage(*MoviePipelineConfigPath);
    NewPackage->MarkAsFullyLoaded();
    //---------------------
    UObject* DuplicateObject = StaticDuplicateObject(Config, NewPackage, FName(*ConfigName), RF_NoFlags);
    UMoviePipelinePrimaryConfig* NewConfig = Cast<UMoviePipelinePrimaryConfig>(DuplicateObject);
    NewConfig->SetFlags(RF_Public | RF_Transactional | RF_Standalone);
    NewConfig->MarkPackageDirty();
    //--------------------------
    FAssetRegistryModule::AssetCreated(NewConfig);
    EditorAssetSubsystem->SaveLoadedAsset(NewConfig);
}

//"D:\\Program Files\\Epic Games\\UE_5.2\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" "D:/Users/Administrator/Documents/Unreal Projects/MyProject/MyProject.uproject" -skipcompile -run=DoodleAutoAnimation  -Params=E:/动画自动导入/DBXY_EP360_SC001_AN/out.json
//UnrealEditor-Cmd.exe D:\\Users\\Administrator\\Documents\\Unreal Projects\\MyProject\\MyProject.uproject -skipcompile -run = DoodleAutoAnimation -Params = E:/动画自动导入/DBXY_EP360_SC001_AN/out.json