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
#include "NiagaraEditor/Public/NiagaraParameterDefinitions.h"
#include "NiagaraParameterCollection.h"

const FName UDoodleEffectLibraryEditWidget::Name{ TEXT("DoodleEffectLibraryEditWidget") };



UDoodleEffectLibraryEditWidget::UDoodleEffectLibraryEditWidget()
{
    EffectName = TEXT("");
    DescText = TEXT("");
    PreviewThumbnail = TEXT("");
    PreviewFile = TEXT("");
    //---------
}

void UDoodleEffectLibraryEditWidget::SetViewportDat()
{
    if (ViewEditorViewport->IsVisible())
    {
        ViewEditorViewport->SetViewportData(SelectObject);
    }
}

void UDoodleEffectLibraryEditWidget::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .FillWidth(1.5)
            [
                SAssignNew(ViewEditorViewport, DoodleEffectEditorViewport)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(0.5)
            [
                SNew(SVerticalBox)
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
                    +SVerticalBox::Slot()
                    .FillHeight(0.1)
                    .Padding(2)
                    [
                        SAssignNew(StartCaptureButton,SButton)
                            .Text(FText::FromString(TEXT("录制")))
                            .OnClicked_Lambda([this]()
                            {
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
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                            .Text(FText::FromString(TEXT("分类：")))
                            .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
                    ]
                    + SVerticalBox::Slot()
                    .Padding(5)
                    .FillHeight(0.1)
                    [
                        SNew(SComboButton)
                            .OnGetMenuContent(this, &UDoodleEffectLibraryEditWidget::OnGetMenuContent)
                            .ContentPadding(FMargin(2.0f, 2.0f))
                            .ButtonContent()
                            [
                                SNew(SEditableTextBox)
                                    .Text_Lambda([this]() 
                                    {
                                        return FText::FromString(EffectType);
                                    })
                                    .OnTextChanged_Lambda([this](const FText& In_Text)
                                    {
                                        EffectType = In_Text.ToString();
                                    })
                                    .OnTextCommitted_Lambda([this](const FText& In_Text, ETextCommit::Type)
                                    {
                                        EffectType = In_Text.ToString();
                                    })
                            ]
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
        TSharedPtr<FSceneViewport> SceneViewport = ViewEditorViewport->GetSceneViewport();
        Capture = NewObject<UDoodleMovieSceneCapture>(GetTransientPackage());
        Capture->Initialize(SceneViewport);
        Capture->Settings.bOverwriteExisting = true;
        //Capture->Settings.bUseCustomFrameRate = true;
        //Capture->Settings.CustomFrameRate = FFrameRate(25,1);
        Capture->Settings.HandleFrames = 0;
        //Capture->Settings.ZeroPadFrameNumbers = 4;
        Capture->Settings.bEnableTextureStreaming = true;
        Capture->Settings.bShowHUD = false;
        Capture->Settings.Resolution = FCaptureResolution(1080,1080);
        Capture->Settings.OutputFormat = OutputFormat;
        DirectoryPath = Capture->Settings.OutputDirectory.Path;
        MovieExtension = Capture->Settings.MovieExtension;
        //---------------
        Capture->OnCaptureFinished().AddRaw(this, &UDoodleEffectLibraryEditWidget::OnCaptureFinished);
        Capture->StartCapture();
    }
}

void UDoodleEffectLibraryEditWidget::OnCaptureFinished()
{
    if (NotificationItem.IsValid()) 
    {
        NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
        NotificationItem->SetText(FText::FromString(TEXT("录制完成")));
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
    IsCapturing = false;
    StartCaptureButton->SetEnabled(true);
    CaptureText->SetText(FText::FromString(TEXT("")));
    if(Capture)
        Capture->Close();
}

void UDoodleEffectLibraryEditWidget::OnTakeThumbnail()
{
    FString FilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Shot"));
    FDelegateHandle ScreenshotHandle = FScreenshotRequest::OnScreenshotRequestProcessed().AddLambda([&, FilePath, ScreenshotHandle]()
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
    if (EffectType.Len() <= 0)
    {
        FText  DialogText = FText::FromString(TEXT("保存失败，分类不能为空。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return;
    }
    //json--------------------
    TSharedPtr<FJsonObject> JsonData = MakeShareable(new FJsonObject);
    JsonData->SetStringField(TEXT("DescText"), DescText);
    JsonData->SetStringField(TEXT("EffectType"), EffectType);
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
        FString Path = FPaths::Combine(TEXT("/Game") / EffectName, SelectObject->GetName());
        TargetObj = EditorAssetSubsystem->DuplicateLoadedAsset(SelectObject, Path);
        if (TargetObj)
        {
            EditorAssetSubsystem->SaveLoadedAsset(TargetObj);
            OnReplaceDependencies(SelectObject, TargetObj);
        }
    }
    //Sort--------------------
    TArray<FAssetData> OutAssetData;
    IAssetRegistry::Get()->GetAssetsByPath(FName(*FPaths::Combine(TEXT("/Game"),EffectName)), OutAssetData, false);
    for (FAssetData Asset : OutAssetData)
    {
        FString Path= Asset.PackagePath.ToString();
        if (Asset.GetClass() == UStaticMesh::StaticClass())
             Path = FPaths::Combine(Path, TEXT("Mesh"));
        if (Asset.GetClass()->IsChildOf<UTexture>())
            Path = FPaths::Combine(Path, TEXT("Tex"));
        if (Asset.GetClass()== UMaterialInstance::StaticClass())
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
        AssetViewUtils::MoveAssets({ Asset.GetAsset() }, Path, Asset.PackagePath.ToString());
    }
    if (TargetObj) 
    {
        TargetObj->Modify();
        EditorAssetSubsystem->SaveLoadedAsset(TargetObj);
    }
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
    //Copy----------------
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (PlatformFile.CopyDirectoryTree(*FPaths::Combine(LibraryPath, EffectName),*FPaths::Combine(FPaths::ProjectContentDir(), EffectName),true))
    {
        IFileManager::Get().DeleteDirectory(*FPaths::Combine(FPaths::ProjectContentDir(), EffectName), false, true);
        //PlatformFile.DeleteDirectoryRecursively(*FPaths::Combine(FPaths::ProjectContentDir(), EffectName));
    }
    EditorAssetSubsystem->DeleteDirectory(*FPaths::Combine(TEXT("/Game"), EffectName));
    //------
    FString Info = FString::Format(TEXT("保存特效：{0}完成"), { EffectName });
    FNotificationInfo L_Info{ FText::FromString(Info) };
    L_Info.FadeInDuration = 1.0f;  // 
    L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
    FSlateNotificationManager::Get().AddNotification(L_Info);
    //Item-------------------------
    TSharedPtr<SDockTab> LibraryTab = FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(UDoodleEffectLibraryWidget::Name));
    if (LibraryTab.IsValid())
    {
        TSharedRef<UDoodleEffectLibraryWidget> Widget = StaticCastSharedRef<UDoodleEffectLibraryWidget>(LibraryTab->GetContent());
        Widget->OnSaveNewEffect(EffectName);
    }
    //----------
    TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(Name));
    if (Tab.IsValid())
    {
        Tab->RequestCloseTab();
    }
}

void UDoodleEffectLibraryEditWidget::OnReplaceDependencies(UObject* SObject, UObject* TObject)
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    TArray<FAssetDependency> Dependencys;
    FName PackageName = FName(SObject->GetPackage()->GetName());
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().GetDependencies(PackageName, Dependencys);
    TMap<UObject*, UObject*> ReplacementMap;
    for (FAssetDependency Dependency : Dependencys)
    {
        UObject* OldObj = LoadObject<UObject>(nullptr, *Dependency.AssetId.PackageName.ToString());
        if (OldObj)
        {
            //-----------------------
            FString FolderPath = FPaths::Combine(TEXT("/Game"), EffectName);
            FString FileName = FPaths::GetBaseFilename(Dependency.AssetId.PackageName.ToString(), true);
            FName TargetPath = FName(FPaths::Combine(FolderPath, FileName));
            while (EditorAssetSubsystem->DoesAssetExist(TargetPath.ToString())) 
            {
                int Counter = TargetPath.GetNumber();
                TargetPath.SetNumber(++Counter);
            }
            UObject* NewObj = EditorAssetSubsystem->DuplicateLoadedAsset(OldObj, TargetPath.ToString());
            EditorAssetSubsystem->SaveLoadedAsset(NewObj);
            if (NewObj)
            {
                ReplacementMap.Add(OldObj, NewObj);
                OnReplaceDependencies(OldObj, NewObj);
            }
        }
    }
    FArchiveReplaceObjectRef<UObject> ReplaceAr(TObject, ReplacementMap, EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::IgnoreArchetypeRef);
    TObject->Modify();
    EditorAssetSubsystem->SaveLoadedAsset(TObject);
}

TSharedRef<SWidget> UDoodleEffectLibraryEditWidget::OnGetMenuContent()
{
    FMenuBuilder MenuBuilder(true, NULL);
    for (int32 i = 0; i < EffectTypeValues.Num(); i++)
    {
        MenuBuilder.AddMenuEntry(FText::FromString(EffectTypeValues[i]), TAttribute<FText>(), FSlateIcon(), 
            FUIAction(FExecuteAction::CreateLambda([this, i]()
            {
                EffectType = EffectTypeValues[i];
            }))
        );
    }
    return MenuBuilder.MakeWidget();
}