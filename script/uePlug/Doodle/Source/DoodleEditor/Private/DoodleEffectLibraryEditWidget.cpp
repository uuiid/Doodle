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
#include "Serialization/ArchiveReplaceObjectAndStructPropertyRef.h"

const FName UDoodleEffectLibraryEditWidget::Name{ TEXT("DoodleEffectLibraryEditWidget") };

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
    Item->Parent = MakeShareable(this);
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
    TSharedPtr<FTypeItem> TempParent = Parent;
    while (TempParent)
    {
        if (TempParent->Name != FName(TEXT("Root")))
            L_Types.EmplaceAt(0, TempParent->Name.ToString());
        TempParent = TempParent->Parent;
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
        LibraryPath = TEXT("E:/EffectLibrary");
    }
    //--------------
    EffectName = TEXT("");
    DescText = TEXT("");
    PreviewThumbnail = TEXT("");
    PreviewFile = TEXT("");
    DirectoryPath = TEXT("");
    OutputFormat = TEXT("Effect");
}

UDoodleEffectLibraryEditWidget::~UDoodleEffectLibraryEditWidget()
{
    if(SelectObject)
        SelectObject->RemoveFromRoot();
}

void UDoodleEffectLibraryEditWidget::SetAssetData(FAssetData Asset)
{
    SelectObject = MakeShareable(Asset.GetAsset());
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
                            .FillHeight(0.1)
                            .Padding(2)
                            [
                                SAssignNew(StartCaptureButton, SButton)
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
                                        FMenuBuilder MenuBuilder(true, false);
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
        Capture->Settings.Resolution = FCaptureResolution(1024,1024);
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
    if (IsCapturing) 
    {
        IsCapturing = false;
        StartCaptureButton->SetEnabled(true);
        CaptureText->SetText(FText::FromString(TEXT("")));
        if (Capture)
            Capture->Close();
    }
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
        EffectTagStr = TEXT("###") + L_Tag->Name;
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
            FStringAssetReference AssetRef1(OldObj);
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
            EditorAssetSubsystem->SaveLoadedAsset(TargetObj1);
        }
    }
    //AssetRegistryModule.Get().ScanPathsSynchronous({ FPaths::ProjectContentDir() }, true);
    ////Sort--------------------
    //OnSortAssetPath(FName(FPaths::Combine(TEXT("/Game"), EffectName)));
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
    EditorAssetSubsystem->DeleteDirectory(*FPaths::Combine(TEXT("/Game"), EffectName));
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
        //Tab->RequestCloseTab();
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