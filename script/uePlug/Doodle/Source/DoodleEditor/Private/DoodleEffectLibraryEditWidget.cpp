// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleEffectLibraryEditWidget.h"

#include "MovieSceneCaptureModule.h"
#include "IMovieSceneCapture.h"
#include "MovieSceneCaptureSettings.h"
#include "HighResScreenshot.h"
#include "Framework/Notifications/NotificationManager.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "DoodleEffectLibraryWidget.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Slate/SceneViewport.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "Serialization/FindReferencersArchive.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "AssetViewUtils.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "NiagaraEmitter.h"
#include "NiagaraParameterDefinitions.h"
#include "NiagaraParameterCollection.h"
#include "Serialization/ArchiveReplaceObjectAndStructPropertyRef.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Async/Async.h"
#include "CineCameraActor.h"
#include "LevelSequenceEditorSubsystem.h"
#include "LevelSequenceActor.h"
#include "MovieSceneToolHelpers.h"
#include "Tracks/MovieSceneSpawnTrack.h"
#include "Sections/MovieSceneSpawnSection.h"
#include "LevelEditorViewport.h"
#include "SequencerTools.h"
#include "SequencerTools.h"
#include "Engine/WorldComposition.h"
#include "UObject/SavePackage.h"
#include "Factories/WorldFactory.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "TimerManager.h"
#include "Engine/TextureCube.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SkyLight.h"
#include "AssetViewerSettings.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "CineCameraComponent.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/SimpleConstructionScript.h"
#include "NiagaraActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "JsonObjectConverter.h"
#include "Serialization/ObjectWriter.h"
#include "Serialization/ObjectReader.h"
#include "Protocols/ImageSequenceProtocol.h"
#include "Protocols/VideoCaptureProtocol.h"

const FName UDoodleEffectLibraryEditWidget::Name{ TEXT("DoodleEffectLibraryEditWidget") };
const TCHAR* MovieCaptureSessionName = TEXT("Movie Scene Capture");

void FInEditorCapture1::Start()
{
    ULevelEditorPlaySettings* PlayInEditorSettings = GetMutableDefault<ULevelEditorPlaySettings>();
    //-----------------
    bScreenMessagesWereEnabled = GAreScreenMessagesEnabled;
    GAreScreenMessagesEnabled = false;

    if (!CaptureObject->Settings.bEnableTextureStreaming)
    {
        const int32 UndefinedTexturePoolSize = -1;
        IConsoleVariable* CVarStreamingPoolSize = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streaming.PoolSize"));
        if (CVarStreamingPoolSize)
        {
            BackedUpStreamingPoolSize = CVarStreamingPoolSize->GetInt();
            CVarStreamingPoolSize->Set(UndefinedTexturePoolSize, ECVF_SetByConsole);
        }
        //---------------------
        IConsoleVariable* CVarUseFixedPoolSize = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streaming.UseFixedPoolSize"));
        if (CVarUseFixedPoolSize)
        {
            BackedUpUseFixedPoolSize = CVarUseFixedPoolSize->GetInt();
            CVarUseFixedPoolSize->Set(0, ECVF_SetByConsole);
        }
        //--------------------
        IConsoleVariable* CVarTextureStreaming = IConsoleManager::Get().FindConsoleVariable(TEXT("r.TextureStreaming"));
        if (CVarTextureStreaming)
        {
            BackedUpTextureStreaming = CVarTextureStreaming->GetInt();
            CVarTextureStreaming->Set(0, ECVF_SetByConsole);
        }
    }
    //-------------------
    FObjectWriter(PlayInEditorSettings, BackedUpPlaySettings);
    OverridePlaySettings(PlayInEditorSettings);

    CaptureObject->AddToRoot();
    CaptureObject->OnCaptureFinished().AddRaw(this, &FInEditorCapture1::OnLevelSequenceFinished);

    UGameViewportClient::OnViewportCreated().AddRaw(this, &FInEditorCapture1::OnPIEViewportStarted);
    FEditorDelegates::EndPIE.AddRaw(this, &FInEditorCapture1::OnEndPIE);

    FVector2D WindowSize = FVector2D(1024, 1024);
    WindowSize = FVector2D(CaptureObject->Settings.Resolution.ResX, CaptureObject->Settings.Resolution.ResY);
    TSharedRef<SWindow> CustomWindow = SNew(SWindow)
        .ClientSize(WindowSize)
        .Title(FText::FromString(TEXT("Movie Render - Preview")))
        .AutoCenter(EAutoCenter::PrimaryWorkArea)
        .UseOSWindowBorder(true)
        .FocusWhenFirstShown(false)
        .ActivationPolicy(EWindowActivationPolicy::Never)
        .HasCloseButton(true)
        .SupportsMaximize(false)
        .SupportsMinimize(true)
        .MaxWidth(CaptureObject->GetSettings().Resolution.ResX)
        .MaxHeight(CaptureObject->GetSettings().Resolution.ResY)
        .SizingRule(ESizingRule::FixedSize);
    FSlateApplication::Get().AddWindow(CustomWindow);
    //----------------
    FRequestPlaySessionParams Params;
    Params.EditorPlaySettings = PlayInEditorSettings;
    Params.CustomPIEWindow = CustomWindow;
    Params.GlobalMapOverride = MapName;
    //-----------------
    GEditor->RequestPlaySession(Params);
}

void FInEditorCapture1::Cancel()
{
    // If the user cancels through the UI then we request that the editor shut down the PIE instance.
    // We capture the PIE shutdown request (which calls OnEndPIE) and further process it. This unifies
    // closing PIE via the close button and the UI into one code path.
    GEditor->RequestEndPlayMap();
}

void FInEditorCapture1::OverridePlaySettings(ULevelEditorPlaySettings* PlayInEditorSettings)
{
    const FMovieSceneCaptureSettings& Settings = CaptureObject->GetSettings();
    PlayInEditorSettings->NewWindowWidth = Settings.Resolution.ResX;
    PlayInEditorSettings->NewWindowHeight = Settings.Resolution.ResY;
    PlayInEditorSettings->CenterNewWindow = false;
    PlayInEditorSettings->NewWindowPosition = FIntPoint::NoneValue; // It will center PIE to the middle of the screen the first time it is run (until the user drag the window somewhere else)
    PlayInEditorSettings->LastExecutedPlayModeType = EPlayModeType::PlayMode_InEditorFloating;

    // Reset everything else
    PlayInEditorSettings->GameGetsMouseControl = false;
    PlayInEditorSettings->ShowMouseControlLabel = false;
    PlayInEditorSettings->ViewportGetsHMDControl = false;
    PlayInEditorSettings->ShouldMinimizeEditorOnVRPIE = true;
    PlayInEditorSettings->EnableGameSound = false;
    PlayInEditorSettings->bOnlyLoadVisibleLevelsInPIE = false;
    PlayInEditorSettings->bPreferToStreamLevelsInPIE = false;
    PlayInEditorSettings->PIEAlwaysOnTop = false;
    PlayInEditorSettings->DisableStandaloneSound = false;
    PlayInEditorSettings->AdditionalLaunchParameters = TEXT("");
    PlayInEditorSettings->BuildGameBeforeLaunch = EPlayOnBuildMode::PlayOnBuild_Never;
    PlayInEditorSettings->LaunchConfiguration = EPlayOnLaunchConfiguration::LaunchConfig_Default;
    PlayInEditorSettings->PackFilesForLaunch = EPlayOnPakFileMode::NoPak;
    PlayInEditorSettings->SetPlayNetMode(EPlayNetMode::PIE_Standalone);
    PlayInEditorSettings->SetRunUnderOneProcess(true);
    PlayInEditorSettings->bLaunchSeparateServer = false;
    PlayInEditorSettings->SetPlayNumberOfClients(1);
}

void FInEditorCapture1::OnPIEViewportStarted()
{
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        if (Context.WorldType == EWorldType::PIE)
        {
            FSlatePlayInEditorInfo* SlatePlayInEditorSession = GEditor->SlatePlayInEditorMap.Find(Context.ContextHandle);
            if (SlatePlayInEditorSession)
            {
                CapturingFromWorld = Context.World();

                TSharedPtr<SWindow> Window = SlatePlayInEditorSession->SlatePlayInEditorWindow.Pin();

                const FMovieSceneCaptureSettings& Settings = CaptureObject->GetSettings();

                SlatePlayInEditorSession->SlatePlayInEditorWindowViewport->SetViewportSize(Settings.Resolution.ResX, Settings.Resolution.ResY);
                //--------------------
                CachedEngineShowFlags = SlatePlayInEditorSession->SlatePlayInEditorWindowViewport->GetClient()->GetEngineShowFlags();
                if (CachedEngineShowFlags && Settings.bUsePathTracer)
                {
                    CachedPathTracingMode = CachedEngineShowFlags->PathTracing;
                    CachedEngineShowFlags->SetPathTracing(true);
                }
                CaptureObject->Initialize(SlatePlayInEditorSession->SlatePlayInEditorWindowViewport, Context.PIEInstance);
                CaptureObject->ImageCaptureProtocolType = UVideoCaptureProtocol::StaticClass();
                //OnCaptureStarted();
            }
            return;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Received PIE Creation callback but failed to find PIE World or missing FSlatePlayInEditorInfo for world."));
}

void FInEditorCapture1::Shutdown()
{
    FEditorDelegates::EndPIE.RemoveAll(this);
    UGameViewportClient::OnViewportCreated().RemoveAll(this);
    CaptureObject->OnCaptureFinished().RemoveAll(this);

    GAreScreenMessagesEnabled = bScreenMessagesWereEnabled;

    if (!CaptureObject->Settings.bEnableTextureStreaming)
    {
        IConsoleVariable* CVarStreamingPoolSize = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streaming.PoolSize"));
        if (CVarStreamingPoolSize)
        {
            CVarStreamingPoolSize->Set(BackedUpStreamingPoolSize, ECVF_SetByConsole);
        }

        IConsoleVariable* CVarUseFixedPoolSize = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streaming.UseFixedPoolSize"));
        if (CVarUseFixedPoolSize)
        {
            CVarUseFixedPoolSize->Set(BackedUpUseFixedPoolSize, ECVF_SetByConsole);
        }

        IConsoleVariable* CVarTextureStreaming = IConsoleManager::Get().FindConsoleVariable(TEXT("r.TextureStreaming"));
        if (CVarTextureStreaming)
        {
            CVarTextureStreaming->Set(BackedUpTextureStreaming, ECVF_SetByConsole);
        }
    }

    if (CachedEngineShowFlags)
    {
        CachedEngineShowFlags->SetPathTracing(CachedPathTracingMode);
    }

    FObjectReader(GetMutableDefault<ULevelEditorPlaySettings>(), BackedUpPlaySettings);

    CaptureObject->Close();
    CaptureObject->RemoveFromRoot();
}

void FInEditorCapture1::OnEndPIE(bool bIsSimulating)
{
    Shutdown();
}

void FInEditorCapture1::OnLevelSequenceFinished()
{
    Shutdown();
    //-----------------
    GEditor->RequestEndPlayMap();
}

void FInEditorCapture1::OnCaptureStarted(){}

FCaptureState FInEditorCapture1::GetCaptureState() const
{
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        if (Context.WorldType == EWorldType::PIE)
        {
            return FCaptureState(ECaptureStatus::Pending);
        }
    }
    return FCaptureState(ECaptureStatus::Success);
}

void FInEditorCapture1::OnCaptureFinished(bool bSuccess)
{
    if (OnFinishedCallback)
    {
        OnFinishedCallback(bSuccess);
    }
}
//-------------------------------------------------------
void FNewProcessCapture1::Start()
{
    // Save out the capture manifest to json
    FString Filename = FPaths::ProjectSavedDir() / TEXT("MovieSceneCapture/Manifest.json");
    //-----------
    TSharedRef<FJsonObject> Object = MakeShareable(new FJsonObject);
    if (FJsonObjectConverter::UStructToJsonObject(CaptureObject->GetClass(), CaptureObject, Object, 0, 0))
    {
        TSharedRef<FJsonObject> RootObject = MakeShareable(new FJsonObject);
        RootObject->SetField(TEXT("Type"), MakeShareable(new FJsonValueString(CaptureObject->GetClass()->GetPathName())));
        RootObject->SetField(TEXT("Data"), MakeShareable(new FJsonValueObject(Object)));

        TSharedRef<FJsonObject> AdditionalJson = MakeShareable(new FJsonObject);
        CaptureObject->SerializeJson(*AdditionalJson);
        RootObject->SetField(TEXT("AdditionalData"), MakeShareable(new FJsonValueObject(AdditionalJson)));

        FString Json;
        TSharedRef<TJsonWriter<> > JsonWriter = TJsonWriterFactory<>::Create(&Json, 0);
        if (FJsonSerializer::Serialize(RootObject, JsonWriter))
        {
            FFileHelper::SaveStringToFile(Json, *Filename);
        }
    }
    else
    {
        return;
    }

    FString EditorCommandLine = FString::Printf(TEXT("%s -MovieSceneCaptureManifest=\"%s\" -game -NoLoadingScreen -ForceRes -Windowed"), *MapNameToLoad, *Filename);

    // Spit out any additional, user-supplied command line args
    if (!CaptureObject->AdditionalCommandLineArguments.IsEmpty())
    {
        EditorCommandLine.AppendChar(' ');
        EditorCommandLine.Append(CaptureObject->AdditionalCommandLineArguments);
    }

    // Spit out any inherited command line args
    if (!CaptureObject->InheritedCommandLineArguments.IsEmpty())
    {
        EditorCommandLine.AppendChar(' ');
        EditorCommandLine.Append(CaptureObject->InheritedCommandLineArguments);
    }

    // Disable texture streaming if necessary
    if (!CaptureObject->Settings.bEnableTextureStreaming)
    {
        EditorCommandLine.Append(TEXT(" -NoTextureStreaming"));
    }

    // Set the game resolution - we always want it windowed
    EditorCommandLine += FString::Printf(TEXT(" -ResX=%d -ResY=%d -Windowed"), CaptureObject->Settings.Resolution.ResX, CaptureObject->Settings.Resolution.ResY);

    // Ensure game session is correctly set up 
    EditorCommandLine += FString::Printf(TEXT(" -messaging -SessionName=\"%s\""), MovieCaptureSessionName);

    FString Params;
    if (FPaths::IsProjectFilePathSet())
    {
        Params = FString::Printf(TEXT("\"%s\" %s %s"), *FPaths::GetProjectFilePath(), *EditorCommandLine, *FCommandLine::GetSubprocessCommandline());
    }
    else
    {
        Params = FString::Printf(TEXT("%s %s %s"), FApp::GetProjectName(), *EditorCommandLine, *FCommandLine::GetSubprocessCommandline());
    }

    FString GamePath = FPlatformProcess::GenerateApplicationPath(FApp::GetName(), FApp::GetBuildConfiguration());
    FProcHandle ProcessHandle = FPlatformProcess::CreateProc(*GamePath, *Params, false, false, false, nullptr, 0, nullptr, nullptr);

    if (ProcessHandle.IsValid())
    {
        if (CaptureObject->bCloseEditorWhenCaptureStarts)
        {
            FPlatformMisc::RequestExit(false);
            return;
        }

        SharedProcHandle = MakeShareable(new FProcHandle(ProcessHandle));
        //OnCaptureStarted();
    }
    else
    {
        OnCaptureFinished(false);
    }
}

void FNewProcessCapture1::Cancel() {}

void FNewProcessCapture1::OnCaptureStarted() {}

FCaptureState FNewProcessCapture1::GetCaptureState() const { return FCaptureState(ECaptureStatus::Pending); }

void FNewProcessCapture1::OnCaptureFinished(bool bSuccess)
{
    if (OnFinishedCallback)
    {
        OnFinishedCallback(bSuccess);
    }
}
//--------------------
void FTypeItemElement::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, const TSharedPtr<FTypeItem> InTreeElement)
{
    WeakTreeElement = InTreeElement;
    FSuperRowType::FArguments SuperArgs = FSuperRowType::FArguments();
    SMultiColumnTableRow::Construct(SuperArgs, InOwnerTable);
}

TSharedRef<SWidget> FTypeItemElement::GenerateWidgetForColumn(const FName& ColumnName)
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(SExpanderArrow, SharedThis(this))
                .IndentAmount(16)
                .ShouldDrawWires(true)
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            SNew(SImage)
                .Image(FAppStyle::GetBrush("ContentBrowser.ColumnViewAssetIcon"))//
        ]
        + SHorizontalBox::Slot()
        .VAlign(VAlign_Center)
        [
            SAssignNew(TheEditableText, SEditableText)
                .MinDesiredWidth(300)
                .IsEnabled(false)
                .OnTextCommitted_Lambda([this](const FText& InText, const ETextCommit::Type InTextAction)
                {
                    if(WeakTreeElement)
                    WeakTreeElement->Name = FName(InText.ToString());
                    if(TheEditableText)
                    TheEditableText->SetEnabled(false);
                })
                .Text(FText::FromName(WeakTreeElement?WeakTreeElement->Name:FName(TEXT(""))))
                .Font(FStyleDefaults::GetFontInfo(12))
        ];
}

FTypeItem::FTypeItem() 
{
    TreeIndex = 0;
    CanEdit = false;
    Name = FName(TEXT(""));
}

TSharedPtr<FTypeItem> FTypeItem::AddChildren(FString L_Name)
{
    TSharedPtr<FTypeItem> Item = MakeShareable(new FTypeItem());
    Item->Name = FName(L_Name);
    Item->Parent = SharedThis(this);
    Item->TreeIndex = TreeIndex + 1;
    Children.Add(Item);
    return Item;
}

TSharedPtr<FTypeItem> FTypeItem::GetChildren(FString L_Name)
{
    TSharedPtr<FTypeItem> L_Item = nullptr;
    for (TSharedPtr<FTypeItem> Item : Children)
    {
        if (Item->Name.IsEqual(FName(L_Name)))
        {
            L_Item = Item;
            break;
        }
    }
    return L_Item;
}

void FTypeItem::ConvertToPath()
{
    TArray<FString>  L_Types;
    L_Types.Insert(Name.ToString(), 0);
    TSharedPtr<FTypeItem> TempParent = Parent.Pin();
    while (TempParent.IsValid())
    {
        if (TempParent->Name != FName(TEXT("Root")))
            L_Types.EmplaceAt(0, TempParent->Name.ToString());
        TempParent = TempParent->Parent.Pin();
    }
    TypePaths.Empty();
    for (FString L_Type : L_Types)
    {
        TypePaths = TypePaths + TEXT("###") + L_Type;
    }
}

UDoodleEffectLibraryEditWidget::UDoodleEffectLibraryEditWidget()
{
    if (!GConfig->GetString(TEXT("DoodleEffectLibrary"), TEXT("EffectLibraryPath"), LibraryPath, GEngineIni))
    {
        //LibraryPath = TEXT("E:/EffectLibrary");
        //---------
        FText  DialogText = FText::FromString(TEXT("没有特效库路径，请在预览界面添加"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
    }
    //--------------
    EffectName = TEXT("");
    DescText = TEXT("");
    PreviewThumbnail = TEXT("");
    PreviewFile = TEXT("");
    DirectoryPath = TEXT("");
    OutputFormat = TEXT("Effect");
    MaxFrame = 99999999;
    IsAutoReset = false;
}

UDoodleEffectLibraryEditWidget::~UDoodleEffectLibraryEditWidget()
{
    if (CaptureSeq) 
    {
        //CaptureSeq->RemoveFromRoot();
        CaptureSeq = nullptr;
    }
    if(SelectObject)
        SelectObject->RemoveFromRoot();
    //--------
    if (ScreenshotHandle.IsValid())
        FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ScreenshotHandle);
    //------
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    if(NewSequenceWorld)
        EditorAssetSubsystem->DeleteLoadedAsset(NewSequenceWorld);
    if(LevelSequence)
        EditorAssetSubsystem->DeleteLoadedAsset(LevelSequence);
}

void UDoodleEffectLibraryEditWidget::SetAssetData(FAssetData Asset)
{
    SelectObject = Asset.GetAsset();
    SelectObject->AddToRoot();
    FName TempEffectName = Asset.AssetName;
    TArray<FName> FileNames;
    IFileManager::Get().IterateDirectory(*LibraryPath, [&](const TCHAR* FilenameOrDirectory, bool bIsDirectory) -> bool
    {
        if (bIsDirectory)
        {
            FString FileName = FPaths::GetCleanFilename(FilenameOrDirectory);
            FileNames.Add(FName(FileName));
        }
        return true;
    });
    while (FileNames.Contains(TempEffectName))
    {
        int Counter = TempEffectName.GetNumber();
        TempEffectName.SetNumber(++Counter);
    }
    EffectName = TempEffectName.ToString();
    //Tag-------------------
    AllEffectTags.Empty();
    IFileManager::Get().IterateDirectory(*LibraryPath, [this](const TCHAR* PathName, bool bIsDir)
    {
        if (bIsDir)
        {
            TSharedPtr<FEffectTileItem> Item = MakeShareable(new FEffectTileItem());
            Item->JsonFile = FPaths::Combine(PathName, TEXT("Data.json"));
            Item->ReadJson();
            for (FString TempTag : Item->EffectTags)
            {
                if (TempTag.Len() > 0 && !AllEffectTags.Contains(TempTag))
                {
                    AllEffectTags.Add(TempTag);
                }
            }
        }
        return true;
    });
    TSharedPtr<FTagItem> TagItem = MakeShareable(new FTagItem());
    if (!AllEffectTags.IsEmpty())
    {
        TagItem->Name = AllEffectTags.Top();
    }
    EffectTags.Add(TagItem);
    //Type---------------------
    RootChildren.Empty();
    TSharedPtr<FTypeItem> RootItem = MakeShareable(new FTypeItem());
    RootItem->Name = FName(TEXT("Root"));
    RootChildren.Add(RootItem);
    IFileManager::Get().IterateDirectory(*LibraryPath, [this](const TCHAR* PathName, bool bIsDir)
    {
        if (bIsDir)
        {
            TSharedPtr<FTypeItem> Parent = RootChildren.Top();
            //--------------
            TSharedPtr<FEffectTileItem> Item = MakeShareable(new FEffectTileItem());
            Item->JsonFile = FPaths::Combine(PathName, TEXT("Data.json"));
            Item->ReadJson();
            int32 LayerIndex = 0;
            while (LayerIndex < Item->EffectTypes.Num())
            {
                FString  LayerType = Item->EffectTypes[LayerIndex];
                TSharedPtr<FTypeItem> LayerItem = Parent->GetChildren(LayerType);
                if (LayerItem == nullptr)
                {
                    LayerItem = Parent->AddChildren(LayerType);
                }
                Parent = LayerItem;
                LayerIndex++;
            }
        }
        return true;
    });
    //---------------------
    if (ViewEditorViewport->IsVisible())
    {
        ViewEditorViewport->SetViewportData(SelectObject);
    }
    CreateLevelSequence();
}

void UDoodleEffectLibraryEditWidget::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SBorder)
                            .BorderBackgroundColor(FColor::Yellow)
                            .Padding(3)
                            [
                                SAssignNew(ViewEditorViewport, DoodleEffectEditorViewport)
                            ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                            .Text(FText::FromString(TEXT("------------------------------")))
                            .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
                    ]
            ]
            + SHorizontalBox::Slot()
            [
                SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    [
                        SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .FillHeight(0.1)
                            .Padding(2)
                            [
                                SNew(SButton)
                                    .Text(FText::FromString(TEXT("重置特效")))
                                    .OnClicked_Lambda([this]()
                                    {
                                        OnResetEffect();
                                        return FReply::Handled();
                                    })
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("略缩图：")))
                                    .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
                            ]
                            + SVerticalBox::Slot()
                            .FillHeight(0.1)
                            .Padding(2)
                            [
                                SAssignNew(StartCaptureButton, SButton)
                                    .Text(FText::FromString(TEXT("截取略缩图")))
                                    .OnClicked_Lambda([this]()
                                        {
                                            OnTakeThumbnail();
                                            return FReply::Handled();
                                        })
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("特效录屏：")))
                                    .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SAssignNew(CaptureText, STextBlock)
                                    .Text(FText::FromString(TEXT("")))
                                    .ColorAndOpacity(FSlateColor{ FLinearColor{1, 0, 0, 1} })
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(SHorizontalBox)
                                +SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString(TEXT("录制时，自动重置粒子：")))
                                ]
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    SNew(SCheckBox)
                                        .IsChecked_Lambda([=, this]() -> ECheckBoxState
                                        {
                                            if (IsAutoReset)
                                                return ECheckBoxState::Checked;
                                            else
                                                return ECheckBoxState::Unchecked;
                                        })
                                        .OnCheckStateChanged_Lambda([=, this](ECheckBoxState NewAutoCloseState)
                                        {
                                            if (NewAutoCloseState == ECheckBoxState::Checked)
                                            {
                                                IsAutoReset = true;
                                            }
                                            else if (NewAutoCloseState == ECheckBoxState::Unchecked)
                                            {
                                                IsAutoReset = false;
                                            }
                                        })
                                ]
                            ]
                            + SVerticalBox::Slot()
                            .FillHeight(0.1)
                            .Padding(2)
                            [
                                SAssignNew(StartCaptureButton, SButton)
                                    .Text(FText::FromString(TEXT("录制")))
                                    .OnClicked_Lambda([this]()
                                    {
                                        if (IsAutoReset) 
                                        {
                                            OnResetEffect();
                                        }
                                        OnStartCapture();
                                        return FReply::Handled();
                                    })
                            ]
                            + SVerticalBox::Slot()
                            .FillHeight(0.1)
                            .Padding(2)
                            [
                                SNew(SButton)
                                    .Text(FText::FromString(TEXT("停止录制")))
                                    .OnClicked_Lambda([this]()
                                    {
                                        OnStopCapture();
                                        return FReply::Handled();
                                    })
                            ]
                            + SVerticalBox::Slot()
                            .FillHeight(0.1)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("特效名：")))
                                    .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
                            ]
                            + SVerticalBox::Slot()
                            .Padding(5)
                            .FillHeight(0.1)
                            [
                                SNew(SEditableTextBox)
                                    .Text_Lambda([this]()-> FText
                                    {
                                        return FText::FromString(EffectName);
                                    })
                                    .OnTextChanged_Lambda([this](const FText& In_Text)
                                    {
                                        EffectName = In_Text.ToString();
                                    })
                                    .OnTextCommitted_Lambda([this](const FText& In_Text, ETextCommit::Type)
                                    {
                                        EffectName = In_Text.ToString();
                                    })
                            ]
                            + SVerticalBox::Slot()
                            .FillHeight(0.1)
                            .Padding(2)
                            [
                                SNew(SButton)
                                    .Text(FText::FromString(TEXT("添加标签")))
                                    .OnClicked_Lambda([this]() 
                                    {
                                        TSharedPtr<FTagItem> Item = MakeShareable(new FTagItem());
                                        EffectTags.Add(Item);
                                        EffectTagsViewPtr->RequestListRefresh();
                                        return FReply::Handled();
                                    })
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SAssignNew(EffectTagsViewPtr, SListView< TSharedPtr<FTagItem> >)
                                    .ListItemsSource(&EffectTags)
                                    .OnGenerateRow(this, &UDoodleEffectLibraryEditWidget::ListOnGenerateRow)
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("描述：")))
                                    .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
                            ]
                            + SVerticalBox::Slot()
                            .FillHeight(0.8)
                            .Padding(5)
                            [
                                SNew(SMultiLineEditableTextBox)
                                    .AllowMultiLine(true)
                                    .AutoWrapText(true)
                                    .Text_Lambda([this]()-> FText
                                    {
                                        return FText::FromString(DescText);
                                    })
                                    .OnTextChanged_Lambda([this](const FText& In_Text)
                                    {
                                        DescText = In_Text.ToString();
                                    })
                                    .OnTextCommitted_Lambda([this](const FText& In_Text, ETextCommit::Type)
                                    {
                                        DescText = In_Text.ToString();
                                    })
                            ]
                            + SVerticalBox::Slot()
                            .FillHeight(0.1)
                            .Padding(2)
                            [
                                SNew(SButton)
                                    .Text(FText::FromString(TEXT("保存并创建")))
                                    .OnClicked_Lambda([this]()
                                    {
                                        OnSaveAndCreate();
                                        return FReply::Handled();
                                    })
                            ]
                            + SVerticalBox::Slot()
                            .FillHeight(0.05)
                    ]
                    + SHorizontalBox::Slot()
                    [
                        SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("分类树：")))
                                    .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
                            ]
                            + SVerticalBox::Slot()
                            [
                                SAssignNew(TreeViewPtr, STreeView<TSharedPtr<FTypeItem>>)
                                    .TreeItemsSource(&RootChildren)
                                    .SelectionMode(ESelectionMode::Single)
                                    .OnGenerateRow(this, &UDoodleEffectLibraryEditWidget::MakeTableRowWidget)
                                    .OnGetChildren(this, &UDoodleEffectLibraryEditWidget::HandleGetChildrenForTree)
                                    .HighlightParentNodesForSelection(true)
                                    .OnSelectionChanged_Lambda([this](TSharedPtr<FTypeItem> inSelectItem, ESelectInfo::Type SelectType)
                                    {
                                        NowSelectType = inSelectItem;
                                    })
                                    .OnContextMenuOpening_Lambda([this]()
                                    {
                                        FUIAction Action(FExecuteAction::CreateLambda([this]() 
                                        {
                                            if (NowSelectType)
                                            {
                                                TSharedPtr<FTypeItem> Item = MakeShareable(new FTypeItem());
                                                TSharedPtr<FTypeItem> NewItem = NowSelectType->AddChildren(TEXT("New"));
                                                NewItem->CanEdit = true;
                                                if (TreeViewPtr)
                                                {
                                                    TreeViewPtr->RequestTreeRefresh();
                                                    TreeViewPtr->SetItemExpansion(Item,true);
                                                }
                                            }
                                        }), FCanExecuteAction());
                                        FUIAction RenameAction(FExecuteAction::CreateLambda([this]()
                                        {
                                            if (NowSelectType &&NowSelectType->CanEdit)
                                            {
                                                TSharedPtr<ITableRow> TableRow = TreeViewPtr->WidgetFromItem(NowSelectType);
                                                TSharedPtr <FTypeItemElement> Row = StaticCastSharedPtr<FTypeItemElement>(TableRow);
                                                if (Row.IsValid())
                                                {
                                                    Row->TheEditableText->SetEnabled(true);
                                                    Row->TheEditableText->SelectAllText();
                                                }
                                            }
                                        }), FCanExecuteAction());
                                        FMenuBuilder MenuBuilder(true, MakeShareable(new FUICommandList));
                                        MenuBuilder.AddMenuSeparator();
                                        MenuBuilder.AddMenuEntry(FText::FromString(TEXT("新建")), FText::FromString(TEXT("新建子分类")),
                                            FSlateIcon(), Action);
                                        if (NowSelectType->CanEdit) 
                                        {
                                            MenuBuilder.AddMenuSeparator();
                                            MenuBuilder.AddMenuEntry(FText::FromString(TEXT("重命名")), FText::FromString(TEXT("重命名分类")),
                                                FSlateIcon(), RenameAction);
                                        }
                                        return MenuBuilder.MakeWidget();
                                    })
                                    .HeaderRow(
                                        SNew(SHeaderRow)
                                        + SHeaderRow::Column(FName(TEXT("Column1")))
                                        .Visibility(EVisibility::Hidden)
                                        .DefaultLabel(FText::FromString(TEXT("")))
                                    )
                            ]
                    ]
            ]
    ];
}

TSharedRef<SDockTab> UDoodleEffectLibraryEditWidget::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(UDoodleEffectLibraryEditWidget)];
}

void UDoodleEffectLibraryEditWidget::OnStartCapture()
{
    if (!IsCapturing)
    {
        IsCapturing = true;
        StartCaptureButton->SetEnabled(false);
        CaptureText->SetText(FText::FromString(TEXT("")));
        CaptureText->SetText(FText::FromString(TEXT("正在录制中...")));
        //------------
        FNotificationInfo Info(FText::FromString(TEXT("录制中...")));
        Info.bUseThrobber = true;
        Info.bFireAndForget = false;
        NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
        NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);
        //-------------------------------------
        StartFrame = PastedFrame;
        CurrentCapture = nullptr;
    }
}

void UDoodleEffectLibraryEditWidget::CreateLevelSequence()
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(SelectObject->GetPathName());
    //World ---------------------
    FString CreateMapPath = FPaths::Combine(AssetData.PackagePath.ToString(), TEXT("Wrold"));
    if (EditorAssetSubsystem->DoesAssetExist(CreateMapPath)) 
    {
        EditorAssetSubsystem->DeleteAsset(CreateMapPath);
    }
    UWorldFactory* Factory = NewObject<UWorldFactory>();
    UPackage* Pkg = CreatePackage(*CreateMapPath);
    Pkg->MarkAsFullyLoaded();
    const FString PackagePath = FPackageName::GetLongPackagePath(CreateMapPath);
    FString BaseFileName = FPaths::GetBaseFilename(CreateMapPath);
    //---------------
    FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    UObject* TempObject = AssetToolsModule.Get().CreateAsset(BaseFileName, PackagePath, UWorld::StaticClass(), Factory);
    NewSequenceWorld = Cast<UWorld>(TempObject);
    AssetRegistryModule.Get().AssetCreated(NewSequenceWorld);
    //Sky -----------------------
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    TObjectPtr<ADirectionalLight> DirectionalLight = NewSequenceWorld->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    DirectionalLight->SetBrightness(1);
    DirectionalLight->GetLightComponent()->SetRelativeRotation(FRotator(-40.0f, -67.5f, 0.0f));
    //---------------------
    UStaticMesh* SkySphere = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorMeshes/AssetViewer/Sphere_inversenormals.Sphere_inversenormals"), NULL, LOAD_None, NULL);
    AStaticMeshActor* SphereActor = NewSequenceWorld->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    SphereActor->SetActorScale3D(FVector(2000.0f, 2000.0f, 2000.0f));
    TObjectPtr<UStaticMeshComponent> SkyComponent = SphereActor->GetComponentByClass<UStaticMeshComponent>();
    SkyComponent->SetStaticMesh(SkySphere);
    SkyComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SkyComponent->CastShadow = false;
    SkyComponent->bCastDynamicShadow = false;
    UMaterial* SkyMaterial = LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorMeshes/ColorCalibrator/M_GreyBall.M_GreyBall"), NULL, LOAD_None, NULL);
    SkyComponent->SetMaterial(0, SkyMaterial);
    //SkyLight ----------------
    UTextureCube* CubemapMap = LoadObject<UTextureCube>(nullptr, TEXT("/Engine/EngineResources/GrayTextureCube.GrayTextureCube"));
    ASkyLight* SkyLightActor = NewSequenceWorld->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    TObjectPtr<USkyLightComponent> SkyLight = SkyLightActor->GetComponentByClass<USkyLightComponent>();
    SkyLight->SetCubemap(CubemapMap);
    SkyLight->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
    SkyLight->SetSourceCubemapAngle(60.f);
    SkyLight->SkyDistanceThreshold = 1.f;
    SkyLight->bLowerHemisphereIsBlack = false;
    //Sequence -------------
    FString FileName = TEXT("Sequence");
    FString SequencePath = FPaths::Combine(AssetData.PackagePath.ToString(), FileName);
    if (EditorAssetSubsystem->DoesAssetExist(SequencePath))
    {
        EditorAssetSubsystem->DeleteAsset(SequencePath);
    }
    UPackage* Package = CreatePackage(*SequencePath);
    LevelSequence = NewObject<ULevelSequence>(Package, *FileName, RF_Public | RF_Standalone | RF_Transactional);
    LevelSequence->Initialize();
    IAssetRegistry::GetChecked().AssetCreated(LevelSequence);
    Package->Modify();
    //-------------------
    const FFrameRate L_Rate{ 25, 1 };
    LevelSequence->GetMovieScene()->SetDisplayRate(L_Rate);
    LevelSequence->GetMovieScene()->SetTickResolutionDirectly(L_Rate);
    //Partile Actor -----------
    AActor* L_Actor = ViewEditorViewport->PreviewActor;
    FGuid L_GUID;
    if (SelectObject->GetClass() == UNiagaraSystem::StaticClass())
    {
        ANiagaraActor* NewActor = NewSequenceWorld->SpawnActorDeferred<ANiagaraActor>(ANiagaraActor::StaticClass(), FTransform::Identity, NULL, NULL, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        UNiagaraSystem* System = CastChecked<UNiagaraSystem>(SelectObject.Get());
        NewActor->GetNiagaraComponent()->SetAsset(System);
        L_GUID = LevelSequence->GetMovieScene()->AddPossessable(NewActor->GetActorLabel(), NewActor->GetClass());
        LevelSequence->BindPossessableObject(L_GUID, *NewActor, NewSequenceWorld);
    }
    if (SelectObject->GetClass() == UParticleSystem::StaticClass())
    {
        AEmitter* NewActor = NewSequenceWorld->SpawnActorDeferred<AEmitter>(AEmitter::StaticClass(), FTransform::Identity, NULL, NULL, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        UParticleSystem* System = CastChecked<UParticleSystem>(SelectObject.Get());
        UParticleSystemComponent* PComponent = NewActor->GetParticleSystemComponent();
        PComponent->SetTemplate(Cast<UParticleSystem>(System));
        PComponent->InitializeSystem();
        PComponent->ActivateSystem();
        L_GUID = LevelSequence->GetMovieScene()->AddPossessable(NewActor->GetActorLabel(), NewActor->GetClass());
        LevelSequence->BindPossessableObject(L_GUID, *NewActor, NewSequenceWorld);
    }
    if (SelectObject->GetClass() == UBlueprint::StaticClass())
    {
        AActor* NewActor = NewSequenceWorld->SpawnActorDeferred<AActor>(L_Actor->GetClass(), L_Actor->GetTransform(), NULL, NULL, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        const EditorUtilities::ECopyOptions::Type CopyOptions = (EditorUtilities::ECopyOptions::Type)(EditorUtilities::ECopyOptions::Default);
        EditorUtilities::CopyActorProperties(L_Actor, NewActor, CopyOptions);
        L_GUID = LevelSequence->GetMovieScene()->AddPossessable(NewActor->GetActorLabel(), NewActor->GetClass());
        LevelSequence->BindPossessableObject(L_GUID, *NewActor, NewSequenceWorld);
    }
    ////--------------
    UMovieSceneSpawnTrack* MovieSceneSpawnTrack = LevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(L_GUID);
    UMovieSceneSpawnSection* MovieSceneSpawnSection = CastChecked<UMovieSceneSpawnSection>(MovieSceneSpawnTrack->CreateNewSection());
    MovieSceneSpawnSection->SetRange(TRange<FFrameNumber>{0, MaxFrame});
    MovieSceneSpawnTrack->AddSection(*MovieSceneSpawnSection);
    //Camera ----------
    UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
    AActor* TempActor = EditorActorSubsystem->SpawnActorFromClass(ACineCameraActor::StaticClass(), FVector::ZAxisVector, FRotator::ZeroRotator, false);
    ACineCameraActor* CameraActor = CastChecked<ACineCameraActor>(LevelSequence->MakeSpawnableTemplateFromInstance(*TempActor, TempActor->GetFName()));
    TSharedPtr<FEditorViewportClient> ViewportClient = ViewEditorViewport->GetViewportClient();
    CameraActor->SetActorLocationAndRotation(ViewportClient->GetViewLocation(), ViewportClient->GetViewRotation());
    CameraActor->GetCineCameraComponent()->Filmback.SensorWidth = 25;
    CameraActor->GetCineCameraComponent()->Filmback.SensorHeight = 25;
    CameraActor->GetCineCameraComponent()->Filmback.RecalcSensorAspectRatio();
    FGuid CameraGuid = LevelSequence->GetMovieScene()->AddSpawnable(CameraActor->GetName(), *CameraActor);
    UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = LevelSequence->GetMovieScene()->AddTrack<UMovieSceneSpawnTrack>(CameraGuid);
    UMovieSceneSpawnSection* L_MovieSceneSpawnSection = CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
    L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);
    FFrameNumber L_Start{ 0 };
    MovieSceneToolHelpers::CreateCameraCutSectionForCamera(LevelSequence->GetMovieScene(), CameraGuid, L_Start);
    UMovieSceneTrack* CameraCutTrack = LevelSequence->GetMovieScene()->GetCameraCutTrack();
    UMovieSceneCameraCutSection* CutSection = CastChecked<UMovieSceneCameraCutSection>(CameraCutTrack->GetAllSections().Top());
    //------------------------
    CutSection->SetCameraGuid(CameraGuid);
    //---------
    TempActor->Destroy();
    NewSequenceWorld->RegisterAutoActivateCamera(CameraActor,0);
    //------------------------
    ALevelSequenceActor* LevelSequenceActor = nullptr;
    ULevelSequencePlayer* LevelSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(NewSequenceWorld, LevelSequence, FMovieSceneSequencePlaybackSettings{}, LevelSequenceActor);
    LevelSequenceActor->InitializePlayer();
    LevelSequencePlayer->Play();
    //-------------------
    NewSequenceWorld->Modify();
    LevelSequence->Modify();
    EditorAssetSubsystem->SaveLoadedAssets({ NewSequenceWorld ,LevelSequence });
    //----------------------
    UWorld* World = ViewEditorViewport->GetViewportClient()->GetWorld();
    PastedFrame = 0;
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindRaw(this, &UDoodleEffectLibraryEditWidget::OnTickTimer);
    World->GetTimerManager().SetTimer(TickTimer, TimerDelegate, 0.2,true);
}

void UDoodleEffectLibraryEditWidget::OnTickTimer()
{
    PastedFrame += 5;
    //----------------
    if (CurrentCapture.IsValid()) 
    {
        FCaptureState StateThisFrame = CurrentCapture->GetCaptureState();
        if (StateThisFrame.Status == ECaptureStatus::Pending)
        {
            if (NotificationItem.IsValid())
                NotificationItem->SetText(FText::FromString(TEXT("渲染中...")));
        }
        else if (StateThisFrame.Status == ECaptureStatus::Success)
        {
            if (CurrentCapture.IsValid())
            {
                CurrentCapture->OnCaptureFinished(true);
            }
        }
        else if(StateThisFrame.Status == ECaptureStatus::Failure)
        {
            IsCapturing = false;
            CurrentCapture = nullptr;
            //--------------
            if (NotificationItem.IsValid())
            {
                NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                NotificationItem->SetText(FText::FromString(TEXT("渲染失败")));
                NotificationItem->ExpireAndFadeout();
                NotificationItem->SetExpireDuration(3);
                NotificationItem = nullptr;
            }
        }
    }
}

void UDoodleEffectLibraryEditWidget::OnCaptureFinished(bool result)
{
    IsCapturing = false;
    if (NotificationItem.IsValid()) 
    {
        CurrentCapture = nullptr;
        NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
        NotificationItem->SetText(FText::FromString(TEXT("渲染完成")));
        NotificationItem->ExpireAndFadeout();
        NotificationItem = nullptr;
    }
    FString FilePath = FPaths::Combine(DirectoryPath, OutputFormat + MovieExtension);
    FString AllFilePath = IFileManager::Get().GetFilenameOnDisk(*FilePath);
    bool FileExists = IFileManager::Get().FileExists(*AllFilePath);
    if (FileExists)
    {
        PreviewFile = AllFilePath;
    }
}

void UDoodleEffectLibraryEditWidget::OnStopCapture()
{
    if (IsCapturing && !CurrentCapture)
    {
        if (NotificationItem.IsValid())
        {
            NotificationItem->SetText(FText::FromString(TEXT("准备渲染...")));
        }
        //---------------
        StartCaptureButton->SetEnabled(true);
        CaptureText->SetText(FText::FromString(TEXT("")));
        EndFrame = PastedFrame;
        LevelSequence->GetMovieScene()->SetPlaybackRange(TRange<FFrameNumber>{StartFrame, EndFrame}, true);
        //Camera----------------
        UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
        TSharedPtr<FEditorViewportClient> ViewportClient = ViewEditorViewport->GetViewportClient();
        UMovieSceneTrack* CameraCutTrack = LevelSequence->GetMovieScene()->GetCameraCutTrack();
        UMovieSceneCameraCutSection* CutSection = CastChecked<UMovieSceneCameraCutSection>(CameraCutTrack->GetAllSections().Top());
        //----------------------
        CutSection->SetRange(TRange<FFrameNumber>{0, EndFrame});
        FMovieSceneObjectBindingID CameraID = CutSection->GetCameraBindingID();
        FMovieSceneSpawnable* Spawnable = LevelSequence->GetMovieScene()->FindSpawnable(CameraID.GetGuid());
        ACineCameraActor* CameraActor = Cast<ACineCameraActor>(Spawnable->GetObjectTemplate());
        CameraActor->SetActorLocationAndRotation(ViewportClient->GetViewLocation(), ViewportClient->GetViewRotation());
        CameraActor->GetCameraComponent()->SetFieldOfView(ViewportClient->ViewFOV);
        LevelSequence->GetMovieScene()->Modify();
        //-----------------------
        TSharedPtr<FSceneViewport> SceneViewport = ViewEditorViewport->GetSceneViewport();
        CaptureSeq = NewObject<UAutomatedLevelSequenceCapture>(GetTransientPackage(), UAutomatedLevelSequenceCapture::StaticClass(), UMovieSceneCapture::MovieSceneCaptureUIName, RF_Transient);
        CaptureSeq->AddToRoot();
        CaptureSeq->LoadFromConfig();
        CaptureSeq->LevelSequenceAsset = LevelSequence;
        CaptureSeq->bUseSeparateProcess = true;
        CaptureSeq->Settings.bOverwriteExisting = true;
        CaptureSeq->Settings.bEnableTextureStreaming = true;
        CaptureSeq->Settings.bShowHUD = false;
        CaptureSeq->Settings.bUseCustomFrameRate = true;
        CaptureSeq->Settings.CustomFrameRate = FFrameRate(25, 1);
        CaptureSeq->Settings.Resolution = FCaptureResolution(1024, 1024);
        CaptureSeq->Settings.OutputFormat = OutputFormat;
        CaptureSeq->ImageCaptureProtocolType = UVideoCaptureProtocol::StaticClass();

        FString TestName = CaptureSeq->Settings.GameModeOverride->GetPathName();
        DirectoryPath = CaptureSeq->Settings.OutputDirectory.Path;
        CaptureSeq->Settings.MovieExtension = TEXT(".avi");
        MovieExtension = CaptureSeq->Settings.MovieExtension;
        auto OnCaptureFinishDelegate = [this](bool bSuccess)
        {
            OnCaptureFinished(bSuccess);
        };
        //------------------
        UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
        EditorAssetSubsystem->SaveLoadedAssets({ LevelSequence,NewSequenceWorld });
        //--------------------
        const FString WorldPackageName = NewSequenceWorld->GetOutermost()->GetName();
        FString MapNameToLoad = WorldPackageName;
        //CurrentCapture = MakeShared<FNewProcessCapture1>(CaptureSeq, WorldPackageName, OnCaptureFinishDelegate);
        //CurrentCapture->Start();
        //--------------------
        CurrentCapture = MakeShared<FInEditorCapture1>(CaptureSeq, OnCaptureFinishDelegate);
        CurrentCapture->MapName = MapNameToLoad;
        CurrentCapture->Start();
    }
}

void UDoodleEffectLibraryEditWidget::OnTakeThumbnail()
{
    FString FilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Shot"));
    ScreenshotHandle = FScreenshotRequest::OnScreenshotRequestProcessed().AddLambda([this, FilePath]()
    {
         PreviewThumbnail = FPaths::ConvertRelativePathToFull(FilePath)+TEXT(".png");
         FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ScreenshotHandle);
    });
    TSharedPtr<FSceneViewport> SceneViewport = ViewEditorViewport->GetSceneViewport();
    FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
    HighResScreenshotConfig.SetFilename(FilePath);
    HighResScreenshotConfig.SetResolution(1024, 1024);
    SceneViewport->TakeHighResScreenShot();
}

void UDoodleEffectLibraryEditWidget::OnSaveAndCreate()
{
    if (PreviewThumbnail.Len() <= 0)
    {
        FText  DialogText = FText::FromString(TEXT("保存失败，请先截取略缩图。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return;
    }
    if (PreviewFile.Len() <= 0)
    {
        FText  DialogText = FText::FromString(TEXT("保存失败，请先录屏。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return;
    }
    if (EffectTags.IsEmpty())
    {
        FText  DialogText = FText::FromString(TEXT("保存失败，请添加标签"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return;
    }
    for (TSharedPtr<FTagItem> L_Tag : EffectTags)
    {
        if (L_Tag->Name.Len() <= 0)
        {
            FText  DialogText = FText::FromString(TEXT("保存失败，标签名不能为空"));
            FMessageDialog::Open(EAppMsgType::Ok, DialogText);
            return;
        }
    }
    if (!NowSelectType|| NowSelectType->Name==FName(TEXT("Root")))
    {
        FText  DialogText = FText::FromString(TEXT("保存失败，请选择分类"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return;
    }
    //Type-----------------------
    NowSelectType->ConvertToPath();
    //json--------------------
    TSharedPtr<FJsonObject> JsonData = MakeShareable(new FJsonObject);
    JsonData->SetStringField(TEXT("DescText"), DescText);
    NowSelectType->ConvertToPath();
    JsonData->SetStringField(TEXT("EffectType"), NowSelectType->TypePaths);
    FString EffectTagStr = TEXT("");
    for (TSharedPtr<FTagItem>& L_Tag : EffectTags)
    {
        EffectTagStr = TEXT("###") + L_Tag->Name + EffectTagStr;
    }
    JsonData->SetStringField("EffectTags", EffectTagStr);
    FString JsonText;
    TSharedRef< TJsonWriter< TCHAR, TPrettyJsonPrintPolicy<TCHAR> > > JsonWriter = TJsonWriterFactory< TCHAR, TPrettyJsonPrintPolicy<TCHAR> >::Create(&JsonText);
    if (FJsonSerializer::Serialize(JsonData.ToSharedRef(), JsonWriter))
    {
        FString Path = FPaths::Combine(FPaths::ProjectContentDir() / EffectName, TEXT("Data.json"));
        FFileHelper::SaveStringToFile(JsonText, *Path);
    }
    //Particle---------------------------
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    UObject* TargetObj = nullptr;
    if (SelectObject)
    {
        ObjectsMap.Empty();
        AllDependens.Empty();
        AllDependens.Add(SelectObject.Get());
        OnGetAllDependencies(SelectObject.Get());
        for (UObject* OldObj:AllDependens)
        {
            FSoftObjectPath AssetRef1(OldObj);
            FName TargetPath = FName(FPaths::Combine(TEXT("/Game") / EffectName, AssetRef1.GetAssetName()));
            while (EditorAssetSubsystem->DoesAssetExist(TargetPath.ToString()))
            {
                int Counter = TargetPath.GetNumber();
                TargetPath.SetNumber(++Counter);
            }
            UObject* NewObj = EditorAssetSubsystem->DuplicateLoadedAsset(OldObj, TargetPath.ToString());
            if(!ObjectsMap.Contains(OldObj))
                ObjectsMap.Add(OldObj, NewObj);
        }
        for (TPair<UObject*, UObject*> TheObject:ObjectsMap)
        {
            TArray<FAssetDependency> Dependencys;
            FName L_PackageName = FName(TheObject.Key->GetPackage()->GetName());
            AssetRegistryModule.Get().GetDependencies(L_PackageName, Dependencys);
            TMap<UObject*, UObject*> ReplacementMap;
            for (FAssetDependency Dependency : Dependencys) 
            {
                UObject* O_Obj = LoadObject<UObject>(nullptr, *Dependency.AssetId.PackageName.ToString());
                if (O_Obj) 
                {
                    UObject* N_Obj = ObjectsMap[O_Obj];
                    ReplacementMap.Add(O_Obj, N_Obj);
                }
            }
            UObject* TargetObj1 = TheObject.Value;
            if (SelectObject.Get()->IsA(UBlueprint::StaticClass()))
            {
                UBlueprint* BPObject = Cast<UBlueprint>(TargetObj1);
                if (BPObject)
                {
                    FArchiveReplaceObjectAndStructPropertyRef<UObject> ReplaceInBPClassObject_Ar(BPObject->GeneratedClass, ReplacementMap, EArchiveReplaceObjectFlags::IncludeClassGeneratedByRef);
                    FArchiveReplaceObjectAndStructPropertyRef<UObject> ReplaceInBPClassDefaultObject_Ar(BPObject->GeneratedClass->ClassDefaultObject, ReplacementMap, EArchiveReplaceObjectFlags::IncludeClassGeneratedByRef);
                    //---------
                }
                FArchiveReplaceObjectAndStructPropertyRef<UObject> ReplaceAr(TargetObj1, ReplacementMap, EArchiveReplaceObjectFlags::IncludeClassGeneratedByRef);
            }
            else
            {
                FArchiveReplaceObjectRef<UObject> ReplaceAr(TargetObj1, ReplacementMap, EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::IgnoreArchetypeRef);
            }
            TargetObj1->Modify();
            //EditorAssetSubsystem->SaveLoadedAsset(TargetObj1);
        }
        //------------------------------
        OnSortAssetPath(FName(FPaths::Combine(TEXT("/Game"), EffectName)));
        for (TPair<UObject*, UObject*> TheObject : ObjectsMap) 
        {
            UObject* TargetObj1 = TheObject.Value;
            UBlueprint* TargetObjBP = Cast<UBlueprint>(TargetObj1);
            if (TargetObjBP) 
            {
                FCompilerResultsLog Results;
                FKismetEditorUtilities::CompileBlueprint(TargetObjBP, EBlueprintCompileOptions::None, &Results);
            }
            TargetObj1->Modify();
            EditorAssetSubsystem->SaveLoadedAsset(TargetObj1);
        }
    }
    //AssetRegistryModule.Get().ScanPathsSynchronous({ FPaths::ProjectContentDir() }, true);
    //Preview------------------
    if (!PreviewFile.IsEmpty())
    {
        FString FullTempEffectPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), EffectName));
        FString File = FPaths::GetCleanFilename(PreviewFile);
        IFileManager::Get().Move(*(FullTempEffectPath / File), *PreviewFile, true);
    }
    //Thumbnail-------------------
    if (!PreviewThumbnail.IsEmpty())
    {
        FString FullTempEffectPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), EffectName));
        FString File = FPaths::GetCleanFilename(PreviewThumbnail);
        IFileManager::Get().Move(*(FullTempEffectPath / File), *PreviewThumbnail, true);
    }
    ////Copy----------------
    AssetRegistryModule.Get().ScanPathsSynchronous({ FPaths::ProjectContentDir() }, true);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (PlatformFile.CopyDirectoryTree(*FPaths::Combine(LibraryPath, EffectName),*FPaths::Combine(FPaths::ProjectContentDir(), EffectName),true))
    {
        IFileManager::Get().DeleteDirectory(*FPaths::Combine(FPaths::ProjectContentDir(), EffectName), false, true);
        //PlatformFile.DeleteDirectoryRecursively(*FPaths::Combine(FPaths::ProjectContentDir(), EffectName));
    }
    FString ThePath = FPaths::Combine(TEXT("/Game"), EffectName);
    if (EditorAssetSubsystem->DoesDirectoryExist(ThePath))
        EditorAssetSubsystem->DeleteDirectory(ThePath);
    //------
    FString Info = FString::Format(TEXT("保存特效：{0}完成"), { EffectName });
    FNotificationInfo L_Info{ FText::FromString(Info) };
    L_Info.FadeInDuration = 1.0f;  // 
    L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
    FSlateNotificationManager::Get().AddNotification(L_Info);
    //Item-------------------------
    //----------
    TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(Name));
    if (Tab)
    {
        Tab->RequestCloseTab();
    }
}

void UDoodleEffectLibraryEditWidget::OnSortAssetPath(FName AssetPath)
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    TArray<FAssetData> OutAssetData;
    IAssetRegistry::Get()->GetAssetsByPath(AssetPath, OutAssetData, false);
    for (FAssetData Asset : OutAssetData)
    {
        FString Path = Asset.PackagePath.ToString();
        if (Asset.GetClass() == UStaticMesh::StaticClass())
            Path = FPaths::Combine(Path, TEXT("Mesh"));
        if (Asset.GetClass()->IsChildOf<UTexture>())
            Path = FPaths::Combine(Path, TEXT("Tex"));
        if (Asset.GetClass()->IsChildOf(UMaterialInstance::StaticClass()))
            Path = FPaths::Combine(Path, TEXT("Mat/MatInst"));
        if (Asset.GetClass() == UMaterial::StaticClass())
            Path = FPaths::Combine(Path, TEXT("Mat/Mat"));
        if (Asset.GetClass() == UMaterialParameterCollection::StaticClass())
            Path = FPaths::Combine(Path, TEXT("Mat/MatParSet"));
        if (Asset.GetClass() == UMaterialFunction::StaticClass())
            Path = FPaths::Combine(Path, TEXT("Mat/MatFun"));
        if (Asset.GetClass() == UNiagaraEmitter::StaticClass())
            Path = FPaths::Combine(Path, TEXT("FX/Emitter"));
        if (Asset.GetClass() == UNiagaraParameterDefinitions::StaticClass())
            Path = FPaths::Combine(Path, TEXT("FX/ParameterDefinitions"));
        if (Asset.GetClass() == UNiagaraParameterCollection::StaticClass())
            Path = FPaths::Combine(Path, TEXT("FX/ParameterCollection"));
        if (Asset.GetClass() == UNiagaraParameterCollectionInstance::StaticClass())
            Path = FPaths::Combine(Path, TEXT("FX/ParameterCollectionIns"));
        if (Asset.GetClass() == UNiagaraEffectType::StaticClass())
            Path = FPaths::Combine(Path, TEXT("FX/EffectType"));
        if (Asset.GetClass() == UNiagaraValidationRuleSet::StaticClass())
            Path = FPaths::Combine(Path, TEXT("FX/ValidationRuleSet"));
        if (Asset.GetClass() == UNiagaraScript::StaticClass())
            Path = FPaths::Combine(Path, TEXT("FX/Script"));
        if (Asset.GetClass() == USkeletalMesh::StaticClass() || Asset.GetClass() == UAnimSequence::StaticClass()
            || Asset.GetClass() == UPhysicsAsset::StaticClass() || Asset.GetClass() == USkeleton::StaticClass())
            Path = FPaths::Combine(Path, TEXT("SK"));
        //--------------------
        FName AssetName = Asset.AssetName;
        while (EditorAssetSubsystem->DoesAssetExist(FPaths::Combine(Path, AssetName.ToString())))
        {
            int Counter = AssetName.GetNumber();
            AssetName.SetNumber(++Counter);
        }
        if (!EditorAssetSubsystem->RenameAsset(Asset.PackageName.ToString(), FPaths::Combine(Path, Asset.AssetName.ToString())))
        {
            FString Info = FString::Format(TEXT("移动文件{0}到{1}失败"), { Asset.PackageName.ToString(), FPaths::Combine(Path, Asset.AssetName.ToString()) });
            UE_LOG(LogTemp, Warning, TEXT("Error: %s"), *Info);
        }
    }
}

void UDoodleEffectLibraryEditWidget::OnGetAllDependencies(UObject* SObject)
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    TArray<FAssetDependency> Dependencys;
    FName L_PackageName = FName(SObject->GetPackage()->GetName());
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().GetDependencies(L_PackageName, Dependencys);
    for (FAssetDependency Dependency : Dependencys)
    {
        UObject* OldObj = LoadObject<UObject>(nullptr, *Dependency.AssetId.PackageName.ToString());
        if (OldObj)
        {
            if (!AllDependens.Contains(OldObj))
            {
                AllDependens.Add(OldObj);
                OnGetAllDependencies(OldObj);
            }
        }
    }
}

TSharedRef<SWidget> UDoodleEffectLibraryEditWidget::OnGetMenuContent(TSharedPtr<FTagItem> InItem)
{
    FMenuBuilder MenuBuilder(true, NULL);
    for (int32 i = 0; i < AllEffectTags.Num(); i++)
    {
        MenuBuilder.AddMenuEntry(FText::FromString(AllEffectTags[i]), TAttribute<FText>(), FSlateIcon(), 
            FUIAction(FExecuteAction::CreateLambda([this,i, InItem]()
            {
                InItem->Name = AllEffectTags[i];
            }))
        );
    }
    return MenuBuilder.MakeWidget();
}

TSharedRef<ITableRow> UDoodleEffectLibraryEditWidget::MakeTableRowWidget(TSharedPtr<FTypeItem> InTreeElement, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(FTypeItemElement, OwnerTable, InTreeElement);
}

void UDoodleEffectLibraryEditWidget::HandleGetChildrenForTree(TSharedPtr<FTypeItem> InItem, TArray<TSharedPtr<FTypeItem>>& OutChildren)
{
    OutChildren.Append(InItem->Children);
}

TSharedRef<ITableRow> UDoodleEffectLibraryEditWidget::ListOnGenerateRow(TSharedPtr<FTagItem> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FTagItem>>, OwnerTable)
        [
            SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                [
                    SNew(SComboButton)
                        .OnGetMenuContent(this, &UDoodleEffectLibraryEditWidget::OnGetMenuContent, InItem)
                        .ContentPadding(FMargin(2.0f, 2.0f))
                        .ButtonContent()
                        [
                            SNew(SEditableTextBox)
                                .Text_Lambda([this, InItem]()
                                {
                                    return FText::FromString(InItem->Name);
                                })
                                .OnTextChanged_Lambda([this, InItem](const FText& In_Text)
                                {
                                    InItem->Name = In_Text.ToString();
                                })
                                .OnTextCommitted_Lambda([this, InItem](const FText& In_Text, ETextCommit::Type)
                                {
                                    InItem->Name = In_Text.ToString();
                                })
                        ]
                ]
                +SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SButton)
                        .Content()
                        [
                            SNew(SImage)
                                .Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete").GetSmallIcon())
                        ]
                        .Text(FText::FromString(TEXT("删除")))
                        .ToolTipText(FText::FromString(TEXT("删除分类")))
                        .OnClicked_Lambda([this,InItem]()
                        {
                            if (EffectTags.Contains(InItem))
                            {
                                EffectTags.Remove(InItem);
                                EffectTagsViewPtr->RequestListRefresh();
                            }
                            return FReply::Handled();
                        })
                ]
            
        ];
}

void UDoodleEffectLibraryEditWidget::OnResetEffect()
{
    if (ViewEditorViewport->IsVisible())
    {
        PastedFrame = 0;
        ViewEditorViewport->OnResetViewport();
    }
}