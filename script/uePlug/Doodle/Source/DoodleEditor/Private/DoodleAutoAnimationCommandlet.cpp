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

//---

#include "Components/DirectionalLightComponent.h"
#include "MoviePipelineAntiAliasingSetting.h"
#include "MoviePipelineGameOverrideSetting.h"
#include "MoviePipelineConsoleVariableSetting.h"
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

	FixMaterialProperty();
	
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
    OnCreateEffectSequenceWorld();
    OnCreateEffectSequence();
    OnCreateSequence();
    //--------------
    ImportPath = JsonObject->GetStringField(TEXT("import_dir"));
    OnCreateSequenceWorld();
    if (!EditorAssetSubsystem->DoesAssetExist(OriginalMapPath.ToString()))
    {
        UE_LOG(LogTemp, Error, TEXT("Original Map Asset File Not Found!"));
        return -1;
    }
    EditorAssetSubsystem->DuplicateAsset(OriginalMapPath.ToString(), RenderMapPath.ToString());
    OnBuildSequence();
    //---------------------
    UWorld* RenderLevel = LoadObject<UWorld>(nullptr, *RenderMapPath.ToString());
    ULevelStreaming* TempStreamingLevel = NewObject<ULevelStreaming>(RenderLevel, ULevelStreamingDynamic::StaticClass(), NAME_None, RF_NoFlags, NULL);
    TempStreamingLevel->SetWorldAsset(TheSequenceWorld);
    RenderLevel->AddStreamingLevel(TempStreamingLevel);
    //---------------
    UMovieSceneLevelVisibilityTrack* NewTrack = TheLevelSequence->GetMovieScene()->FindTrack<UMovieSceneLevelVisibilityTrack>();
    if (NewTrack) 
    {
        TheLevelSequence->GetMovieScene()->RemoveTrack(*Cast<UMovieSceneTrack>(NewTrack));
    }
    NewTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneLevelVisibilityTrack>();
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
    //-------------
    LevelNames.Add(EffectMapPath);
    NewSection->SetLevelNames(LevelNames);
    //----------------------------------------
    UMovieSceneSubTrack* NewTrack1 = TheLevelSequence->GetMovieScene()->FindTrack<UMovieSceneSubTrack>();
    if (NewTrack1) 
    {
        TheLevelSequence->GetMovieScene()->RemoveTrack(*Cast<UMovieSceneTrack>(NewTrack1));
    }
    NewTrack1 = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSubTrack>();
    FFrameNumber Duration = ConvertFrameTime(
        TheLevelSequence->GetMovieScene()->GetPlaybackRange().Size<FFrameNumber>(),
        TheLevelSequence->GetMovieScene()->GetTickResolution(),
        TheLevelSequence->GetMovieScene()->GetTickResolution()).FloorToFrame();
    UMovieSceneSubSection* NewSection1 = CastChecked<UMovieSceneSubSection>(NewTrack1->CreateNewSection());
    NewSection1->SetSequence(EffectLevelSequence);
    NewSection1->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
    NewTrack1->AddSection(*NewSection1);
    //---------------------
    TheLevelSequence->Modify();
    //NewSection->SetVisibility(ELevelVisibility::Visible);
    EditorAssetSubsystem->SaveLoadedAssets({ TheLevelSequence,RenderLevel });
    //-----------------
    OnSaveReanderConfig();
    return 0;
}

void UDoodleAutoAnimationCommandlet::OnCreateEffectSequence() 
{
    EffectSequencePath = JsonObject->GetStringField(TEXT("level_sequence_vfx"));
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
    FString AssetName = FPaths::GetBaseFilename(EffectSequencePath);
    EffectLevelSequence = LoadObject<ULevelSequence>(nullptr, *(EffectSequencePath));
    if (!EffectLevelSequence)
    {
        UPackage* Package = CreatePackage(*EffectSequencePath);
        EffectLevelSequence = NewObject<ULevelSequence>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
        EffectLevelSequence->Initialize();
        IAssetRegistry::GetChecked().AssetCreated(EffectLevelSequence);
        Package->Modify();
    }
    //------------
    const FFrameRate L_Rate{ 25, 1 };
    FFrameNumber Offset{ 0 };
    L_Start = FFrameNumber(JsonObject->GetIntegerField(TEXT("begin_time")));
    L_End = FFrameNumber(JsonObject->GetIntegerField(TEXT("end_time")));
    //--------------------------
    EffectLevelSequence->GetMovieScene()->SetDisplayRate(L_Rate);
    EffectLevelSequence->GetMovieScene()->SetTickResolutionDirectly(L_Rate);
    //--------------------
    EffectLevelSequence->GetMovieScene()->SetWorkingRange((L_Start - 30 - Offset) / L_Rate, (L_End + 30) / L_Rate);
    EffectLevelSequence->GetMovieScene()->SetViewRange((L_Start - 30 - Offset) / L_Rate, (L_End + 30) / L_Rate);
    EffectLevelSequence->GetMovieScene()->SetPlaybackRange(TRange<FFrameNumber>{L_Start - Offset, L_End + 1}, true);
    EffectLevelSequence->GetMovieScene()->Modify();
    //----------------
    TArray<FMovieSceneBinding> Bindings = EffectLevelSequence->GetMovieScene()->GetBindings();
    for (FMovieSceneBinding Bind : Bindings)
    {
        if (!EffectLevelSequence->GetMovieScene()->RemovePossessable(Bind.GetObjectGuid()))
        {
            EffectLevelSequence->GetMovieScene()->RemoveSpawnable(Bind.GetObjectGuid());
        }
    }
    TArray<UMovieSceneTrack*> Tracks = EffectLevelSequence->GetMovieScene()->GetTracks();
    for (UMovieSceneTrack* Track : Tracks)
    {
        FString Name = Track->GetClass()->GetName();
        EffectLevelSequence->GetMovieScene()->RemoveTrack(*Track);
    }
    EditorAssetSubsystem->SaveLoadedAsset(EffectLevelSequence);
}

void UDoodleAutoAnimationCommandlet::OnCreateEffectSequenceWorld() 
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    EffectMapPath = FName(JsonObject->GetStringField(TEXT("vfx_map")));
    EffectSequenceWorld = LoadObject<UWorld>(nullptr, *EffectMapPath.ToString());
    if (!EffectSequenceWorld)
    {
        UWorldFactory* Factory = NewObject<UWorldFactory>();
        UPackage* Pkg = CreatePackage(*EffectMapPath.ToString());
        Pkg->FullyLoad();
        Pkg->MarkPackageDirty();
        const FString PackagePath = FPackageName::GetLongPackagePath(EffectMapPath.ToString());
        FString BaseFileName = FPaths::GetBaseFilename(EffectMapPath.ToString());
        //---------------
        FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
        UObject* TempObject = AssetToolsModule.Get().CreateAsset(BaseFileName, PackagePath, UWorld::StaticClass(), Factory);
        EffectSequenceWorld = Cast<UWorld>(TempObject);
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        AssetRegistryModule.Get().AssetCreated(EffectSequenceWorld);
        //------------------------
        EffectSequenceWorld->Modify();
        EditorAssetSubsystem->SaveLoadedAsset(EffectSequenceWorld);
    }
    //--------------------
    ULevel* Level = EffectSequenceWorld->GetCurrentLevel();
    for (int32 Index = 0; Index < Level->Actors.Num(); Index++)
    {
        AActor* Actor = Level->Actors[Index];
        if (Actor != nullptr)
        {
            Actor->Destroy();
        }
    }
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
    TheLevelSequence->GetMovieScene()->SetPlaybackRange(TRange<FFrameNumber>{L_Start - Offset, L_End+1}, true);
    TheLevelSequence->GetMovieScene()->Modify();
    //----------------
    TArray<FMovieSceneBinding> Bindings = TheLevelSequence->GetMovieScene()->GetBindings();
    for (FMovieSceneBinding Bind : Bindings)
    {
        if (!Bind.GetName().Contains(TEXT("Camera"))) 
        {
            if (!TheLevelSequence->GetMovieScene()->RemovePossessable(Bind.GetObjectGuid()))
            {
                TheLevelSequence->GetMovieScene()->RemoveSpawnable(Bind.GetObjectGuid());
            }
        }
    }
    TArray<UMovieSceneTrack*> Tracks = TheLevelSequence->GetMovieScene()->GetTracks();
    for (UMovieSceneTrack* Track : Tracks)
    {
        FString Name = Track->GetClass()->GetName();
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
        if (Actor != nullptr)
        {
            if(Actor->IsA<ASkeletalMeshActor>()|| Actor->IsA<AGeometryCacheActor>()
                || Actor->IsA<ADirectionalLight>())
                Actor->Destroy();
        }
    }
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

void UDoodleAutoAnimationCommandlet::OnBuildSequence()
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
    TArray<TSharedPtr<FJsonValue>> JsonFiles = JsonObject->GetArrayField(TEXT("files"));
    TArray<UGeometryCache*> GeometryCaches;
    for (TSharedPtr<FJsonValue> JsonFile : JsonFiles)
    {
        TSharedPtr<FJsonObject> Obj = JsonFile->AsObject();
        const FString Path = Obj->GetStringField(TEXT("path"));
        if (FPaths::FileExists(Path))
        {
            const FString Type = Obj->GetStringField(TEXT("type"));
            if (Type == TEXT("cam"))
            {
                TArray<FMovieSceneBinding> Bindings = TheLevelSequence->GetMovieScene()->GetBindings();
                for (FMovieSceneBinding Bind : Bindings)
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
                //----------------------------
                Bindings = TheLevelSequence->GetMovieScene()->GetBindings();
                for (FMovieSceneBinding Bind : Bindings)
                {
                    if (Bind.GetName() == CameraLabel)
                    {
                        UMovieScene3DTransformTrack* TransformTrack = TheLevelSequence->GetMovieScene()->FindTrack<UMovieScene3DTransformTrack>(Bind.GetObjectGuid());
                        for (UMovieSceneSection* TheSections : TransformTrack->GetAllSections())
                        {
                            UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(TheSections);
                            if (TransformSection)
                            {
                                FMovieSceneChannelProxy& SectionChannelProxy = TransformSection->GetChannelProxy();
                                TMovieSceneChannelHandle<FMovieSceneDoubleChannel> DoubleChannels[] = {
                                    SectionChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>("Rotation.X"),
                                    SectionChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>("Rotation.Y"),
                                    SectionChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>("Rotation.Z")
                                };
                                FMovieSceneDoubleChannel* ChannelX = DoubleChannels[0].Get();
                                FMovieSceneDoubleChannel* ChannelY = DoubleChannels[1].Get();
                                FMovieSceneDoubleChannel* ChannelZ = DoubleChannels[2].Get();
                                ChannelX->Evaluate(1001, CameraRot.Roll);
                                ChannelY->Evaluate(1001, CameraRot.Pitch);
                                ChannelZ->Evaluate(1001, CameraRot.Yaw);
                            }
                        }
                    }
                }
                DirectionalLight1->SetActorRotation(FRotator(CameraRot.Pitch, CameraRot.Yaw-40, CameraRot.Roll));
                DirectionalLight2->SetActorRotation(FRotator(CameraRot.Pitch, CameraRot.Yaw+40, CameraRot.Roll));
            }
            else if(Type == TEXT("char"))
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
				K_FBX_F->ImportUI->SkeletalMeshImportData->bUseT0AsRefPose     = true;
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
            else if (Type == TEXT("geo"))
            {
                UAssetImportTask* Task = NewObject<UAssetImportTask>();
                Task->AddToRoot();
                Task->bAutomated = true;
                //L_Data->GroupName = TEXT("doodle import");
                Task->Filename = Path;
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
                k_abc_stting->SamplingSettings.bSkipEmpty = true;       //
                k_abc_stting->SamplingSettings.FrameStart = L_Start.Value;  //
                k_abc_stting->SamplingSettings.FrameEnd = L_End.Value;    // 
                k_abc_stting->SamplingSettings.FrameSteps = 1;          //
                //------------
                k_abc_f->SetAssetImportTask(Task);
                ImportTasksAbc.Add(Task);
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
            if (!ImportedObjs.IsEmpty())
            {
                UObject* ImportedObject = ImportedObjs.Top();;
                if (ImportedObject->GetClass()->IsChildOf(USkeletalMesh::StaticClass()))
                {
                    USkeletalMesh* TmpSkeletalMesh = Cast<USkeletalMesh>(ImportedObject);
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
                            AnimSeq->CompressCommandletVersion = 0;
                            AnimSeq->ClearAllCachedCookedPlatformData();
                            AnimSeq->CacheDerivedDataForCurrentPlatform();
                        }
                        //------------------
                        ASkeletalMeshActor* L_Actor = TheSequenceWorld->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
                        L_Actor->SetActorLabel(TmpSkeletalMesh->GetName());
                        L_Actor->GetSkeletalMeshComponent()->SetSkeletalMesh(TmpSkeletalMesh);
                        L_Actor->GetSkeletalMeshComponent()->SetLightingChannels(false,true,false);
                        //---------------------
                        const FGuid L_GUID = TheLevelSequence->GetMovieScene()->AddPossessable(L_Actor->GetActorLabel(), L_Actor->GetClass());
                        TheLevelSequence->BindPossessableObject(L_GUID, *L_Actor,TheSequenceWorld);
                        //-----------------------
                        UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(L_GUID);
                        UMovieSceneSpawnSection* L_MovieSceneSpawnSection = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
                        L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);
                        L_MovieSceneSpawnSection->SetRange(TheLevelSequence->GetMovieScene()->GetPlaybackRange());
                        UMovieSceneSkeletalAnimationTrack* L_MovieSceneSkeletalAnim = TheLevelSequence->GetMovieScene()->AddTrack<UMovieSceneSkeletalAnimationTrack>(L_GUID);
                        UMovieSceneSection* AnimSection = L_MovieSceneSkeletalAnim->AddNewAnimationOnRow(L_Start, AnimSeq, -1);
                        AnimSection->Modify();
                        //--------------------------Clone------------------------
                        ASkeletalMeshActor* L_Actor2 = TheSequenceWorld->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
                        L_Actor2->SetActorLabel(TmpSkeletalMesh->GetName()+TEXT("_SH"));
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
    AssetTools.Get().ImportAssetTasks(ImportTasksAbc); 
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
                    L_Actor->Modify();
                    //---------------------------Clone----------------------------
                    AGeometryCacheActor* L_Actor2 = TheSequenceWorld->SpawnActor<AGeometryCacheActor>(FVector::ZeroVector, FRotator::ZeroRotator);
                    L_Actor2->SetActorLabel(TempGeometryCache->GetName()+TEXT("_SH"));
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
    EditorAssetSubsystem->SaveLoadedAssets({ TheLevelSequence,TheSequenceWorld });
}

void UDoodleAutoAnimationCommandlet::OnSaveReanderConfig()
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    DestinationPath                             = JsonObject->GetStringField(TEXT("out_file_dir"));
    MoviePipelineConfigPath                     = JsonObject->GetStringField(TEXT("movie_pipeline_config"));
    FString ConfigName = FPaths::GetBaseFilename(MoviePipelineConfigPath);
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
    UMoviePipelineOutputSetting* OutputSetting =
        Cast<UMoviePipelineOutputSetting>(Config->FindOrAddSettingByClass(UMoviePipelineOutputSetting::StaticClass()));
    OutputSetting->OutputDirectory.Path                  = DestinationPath;
    UMoviePipelineImageSequenceOutput_JPG* FormatSetting = Config->FindSetting<UMoviePipelineImageSequenceOutput_JPG>();
    if (FormatSetting) {
        Config->RemoveSetting(FormatSetting);
    }
    Config->FindOrAddSettingByClass(UMoviePipelineImageSequenceOutput_PNG::StaticClass());
    Config->FindOrAddSettingByClass(UMoviePipelineDeferredPassBase::StaticClass());

    // 设置抗拒齿方法
    UMoviePipelineAntiAliasingSetting* AntiAliasing = Cast<UMoviePipelineAntiAliasingSetting>(
        Config->FindOrAddSettingByClass(UMoviePipelineAntiAliasingSetting::StaticClass())
    );
    if (AntiAliasing) {
        AntiAliasing->SpatialSampleCount    = 1;
        AntiAliasing->TemporalSampleCount   = 1;
        AntiAliasing->bOverrideAntiAliasing = true;
        AntiAliasing->AntiAliasingMethod    = EAntiAliasingMethod::AAM_TSR;
        AntiAliasing->bRenderWarmUpFrames   = true;
        AntiAliasing->EngineWarmUpCount     = 64;
    }
    if (UMoviePipelineGameOverrideSetting* GameOver = Cast<UMoviePipelineGameOverrideSetting>(
            Config->FindOrAddSettingByClass(UMoviePipelineGameOverrideSetting::StaticClass())
        );
        GameOver) {
        GameOver->bCinematicQualitySettings = false;
    }
	if (UMoviePipelineConsoleVariableSetting* ConsoleVar = Cast<UMoviePipelineConsoleVariableSetting>(
                Config->FindOrAddSettingByClass(UMoviePipelineConsoleVariableSetting::StaticClass())
            )) {
        Config->RemoveSetting(ConsoleVar);
	}

    //-------------------------Save
    Config->SetFlags(RF_Public | RF_Transactional | RF_Standalone);
    Config->MarkPackageDirty();
    //--------------------------
    FAssetRegistryModule::AssetCreated(Config);
    EditorAssetSubsystem->SaveLoadedAsset(Config);
}

void UDoodleAutoAnimationCommandlet::FixMaterialProperty() {
    FARFilter LFilter{};
    LFilter.bIncludeOnlyOnDiskAssets = false;
    LFilter.bRecursivePaths          = true;
    LFilter.bRecursiveClasses        = true;
    LFilter.PackagePaths             = TArray<FName>{FName{TEXT("/Game/Character/")}, FName{TEXT("/Game/Prop/")}};
    LFilter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());

    TArray<UObject*> L_Save{};

    IAssetRegistry::Get()->EnumerateAssets(LFilter, [&](const FAssetData& InAss) -> bool {
      if (FPaths::IsUnderDirectory(InAss.PackagePath.ToString(), TEXT("/Game/Character/")) ||
          FPaths::IsUnderDirectory(InAss.PackagePath.ToString(), TEXT("/Game/Prop/"))) {
        if (UMaterial* L_Mat = Cast<UMaterial>(InAss.GetAsset())) {
          bool L_bHasProperty{true};
          // L_Mat->SetMaterialUsage(L_bHasProperty, EMaterialUsage::MATUSAGE_GeometryCache);
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
//UnrealEditor-Cmd.exe D:\\Users\\Administrator\\Documents\\Unreal Projects\\MyProject\\MyProject.uproject -skipcompile -run=DoodleAutoAnimation  -Params=E:/AnimationImport/DBXY_EP360_SC001_AN/out.json