// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleEffectLibraryWidget.h"

#include "IContentBrowserDataModule.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "MovieSceneCaptureModule.h"
#include "IMovieSceneCapture.h"
#include "Slate/SceneViewport.h"
#include "Engine/GameEngine.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "HighResScreenshot.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "MovieSceneCaptureSettings.h"
#include "MovieSceneCaptureDialogModule.h"
#include "ImageUtils.h"
#include "Widgets/Notifications/INotificationWidget.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Images/SThrobber.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Input/SDirectoryPicker.h"

#include "Widgets/Layout/SUniformGridPanel.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "Particles/ParticleSystem.h"
#include "NiagaraSystem.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "AssetViewUtils.h"

const FName UDoodleEffectLibraryWidget::Name{ TEXT("DoodleEffectLibraryWidget") };

class SPlayPreviewDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPlayPreviewDialog)
        {}
    SLATE_END_ARGS()
    //--------------
private:
    TObjectPtr<UMediaPlayer> MediaPlayer;
    TObjectPtr<UMediaTexture> MediaTexture;

public:
    void Construct(const FArguments& InArgs)
    {
        ChildSlot
            [
                SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("Menu.Background"))
                    [
                        SNew(SOverlay)
                            + SOverlay::Slot()
                            [
                                SNew(SMediaImage, MediaTexture)
                            ]
                    ]
            ];
    }

    SPlayPreviewDialog()
    {
        MediaPlayer = NewObject<UMediaPlayer>(GetTransientPackage(), NAME_None, RF_Transient);
        MediaPlayer->AddToRoot();
        MediaPlayer->SetLooping(true);
        //----------
        MediaTexture = NewObject<UMediaTexture>(GetTransientPackage(), NAME_None, RF_Transient);
        MediaTexture->AddToRoot();
        MediaTexture->SetDefaultMediaPlayer(MediaPlayer.Get());
        MediaTexture->UpdateResource();
        MediaTexture->ClearColor = FLinearColor::Transparent;
    }

    ~SPlayPreviewDialog() 
    {
        if (MediaPlayer)
            MediaPlayer->RemoveFromRoot();
        if (MediaTexture)
            MediaTexture->RemoveFromRoot();
    }

    void Show(FString PreviewFile)
    {
        TObjectPtr<UFileMediaSource> TheMediaSource = NewObject<UFileMediaSource>(GetTransientPackage());
        TheMediaSource->SetFilePath(PreviewFile);
        MediaPlayer->SetDesiredPlayerName(TEXT("VlcMedia"));
        if (MediaPlayer->CanPlaySource(TheMediaSource.Get()))
        {
            MediaPlayer->OpenSource(TheMediaSource.Get());
        }
    }
};

class SCreateEffectDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCreateEffectDialog)
        {}
    SLATE_END_ARGS()
    //--------------
private:
    TSharedPtr<SVerticalBox> Container;
    TWeakPtr<SWindow> PickerWindow;
public:
    FAssetData TheAssetData;

    void Construct(const FArguments& InArgs)
    {
        ChildSlot
            [
                SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("Menu.Background"))
                    [
                        SNew(SBox)
                            .WidthOverride(600.0f)
                            [
                                SNew(SVerticalBox)
                                    + SVerticalBox::Slot()
                                    .FillHeight(1)
                                    .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                                    [
                                        SNew(SBorder)
                                            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                                            .Content()
                                            [
                                                SAssignNew(Container, SVerticalBox)
                                            ]
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .HAlign(HAlign_Right)
                                    .VAlign(VAlign_Bottom)
                                    .Padding(8)
                                    [
                                        SNew(SUniformGridPanel)
                                            .SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
                                            .MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
                                            .MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
                                            + SUniformGridPanel::Slot(0, 0)
                                            [
                                                SNew(SButton)
                                                    .HAlign(HAlign_Center)
                                                    .ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
                                                    .OnClicked_Lambda([this]()
                                                    {
                                                        if (PickerWindow.IsValid())
                                                        {
                                                            PickerWindow.Pin()->RequestDestroyWindow();
                                                        }
                                                        return FReply::Handled();
                                                    })
                                                    .Text(FText::FromString(TEXT("Ok")))
                                            ]
                                            + SUniformGridPanel::Slot(1, 0)
                                            [
                                                SNew(SButton)
                                                    .HAlign(HAlign_Center)
                                                    .ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
                                                    .OnClicked_Lambda([this]()
                                                    {
                                                        TheAssetData = nullptr;
                                                        if (PickerWindow.IsValid())
                                                        {
                                                            PickerWindow.Pin()->RequestDestroyWindow();
                                                        }
                                                        return FReply::Handled();
                                                    })
                                                    .Text(FText::FromString(TEXT("Cancel")))
                                            ]
                                    ]
                            ]
                    ]
            ];
        //-----------------------------------------------------
        FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
        FAssetPickerConfig AssetPickerConfig;
        AssetPickerConfig.Filter.ClassPaths.Add(FTopLevelAssetPath{ UParticleSystem::StaticClass()->GetPathName() });
        AssetPickerConfig.Filter.ClassPaths.Add(FTopLevelAssetPath{ UNiagaraSystem::StaticClass()->GetPathName() });
        AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateLambda([this](const FAssetData& AssetData)
        {
            TheAssetData = AssetData;
        });
        AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateLambda([](const FAssetData& AssetData) {return false;});
        AssetPickerConfig.bAllowNullSelection = true;
        AssetPickerConfig.InitialAssetViewType = EAssetViewType::Column;
        AssetPickerConfig.InitialAssetSelection = TheAssetData;
        //-------------------------
        Container->ClearChildren();
        Container->AddSlot()
        .AutoHeight()
        [
            SNew(STextBlock)
                .Text(FText::FromString(TEXT("目标特效:")))
                .ShadowOffset(FVector2D(1.0f, 1.0f))
        ];
        Container->AddSlot()
        [
            ContentBrowserModule.Get()
                .CreateAssetPicker(AssetPickerConfig)
        ];
    }

    void Show()
    {
        TSharedRef<SWindow> Window = SNew(SWindow)
            .Title(FText::FromString(TEXT("创建特效")))
            .ClientSize(FVector2D{ 800,700 })
            .SupportsMinimize(false)
            .SupportsMaximize(false)
            [
                SharedThis(this)
            ];
        //--------------------------
        PickerWindow = Window;
        GEditor->EditorAddModalWindow(Window);
    }
};

UDoodleEffectLibraryWidget::UDoodleEffectLibraryWidget() 
{
    EffectType = TEXT("全部");
    FilterText = TEXT("");
    if (!GConfig->GetString(TEXT("DoodleEffectLibrary"), TEXT("EffectLibraryPath"), LibraryPath, GEngineIni)) 
    {
        LibraryPath = TEXT("E:/EffectLibrary");
    }
    //---------------
    AllTreeRootItems.Empty();
    TreeRootItems.Empty();
    IFileManager::Get().IterateDirectory(*LibraryPath, [this](const TCHAR* PathName, bool bIsDir)
    {
        if (bIsDir)
        {
            TSharedPtr<FEffectTreeItem> Item = CreateTreeItem(PathName);
            Item->ReadJson();
            AllTreeRootItems.Add(Item);
            TreeRootItems.Add(Item);
        }
        return true; // Continue
    });
}

UDoodleEffectLibraryWidget::~UDoodleEffectLibraryWidget() 
{
}

void UDoodleEffectLibraryWidget::Construct(const FArguments& InArgs)
{
    ChildSlot
        [
            SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                        .Text(FText::FromString(TEXT("双击播放预览：")))
                        .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
                ]
                + SVerticalBox::Slot()
                .Padding(2)
                .FillHeight(0.1)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SComboButton)
                                .ForegroundColor(FLinearColor::White)
                                .ContentPadding(0)
                                .ToolTipText(FText::FromString(TEXT("特效类别:")))
                                .OnGetMenuContent(this, &UDoodleEffectLibraryWidget::MakeAddFilterMenu)
                                .HasDownArrow(true)
                                .ContentPadding(FMargin(1, 0))
                                .AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ExtContentBrowserFiltersCombo")))
                                .Visibility(EVisibility::Visible)
                                .ButtonContent()
                                [
                                    SNew(SHorizontalBox)

                                        + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        [
                                            SNew(STextBlock)
                                                .TextStyle(FAppStyle::Get(), "GenericFilters.TextStyle")
                                                .Font(FAppStyle::Get().GetFontStyle("FontAwesome.9"))
                                                .Text(FText::FromString(FString(TEXT("\xf0b0"))) /*fa-filter*/)
                                        ]

                                        + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        .Padding(2, 0, 0, 0)
                                        [
                                            SNew(STextBlock)
                                                .TextStyle(FAppStyle::Get(), "GenericFilters.TextStyle")
                                                .Text_Lambda([this]()-> FText 
                                                {
                                                    return FText::FromString(this->EffectType);
                                                })
                                        ]
                                ]
                        ]
                        + SHorizontalBox::Slot()
                        .Padding(10)
                        .FillWidth(0.6f)
                        [
                            SAssignNew(SearchBoxPtr, SAssetSearchBox)
                                .HintText(FText::FromString(TEXT("搜索特效名")))
                                .OnTextCommitted(this, &UDoodleEffectLibraryWidget::OnSearchBoxCommitted)
                                .DelayChangeNotificationsWhileTyping(true)
                                .AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ExtContentBrowserSearchAssets")))
                                .ShowSearchHistory(true)
                        ]
                        +SHorizontalBox::Slot()
                        .FillWidth(0.1f)
                        [
                            SNew(SButton)
                                .Text(FText::FromString(TEXT("新建特效")))
                                .OnClicked_Lambda([this]()
                                {
                                    OnCreateNewEffect();
                                    return FReply::Handled();
                                })
                        ]
                ]
                + SVerticalBox::Slot()
                .Padding(2.0f)
                [
                    SAssignNew(TileViewPtr, STileView<TSharedPtr<FEffectTreeItem>>)
                        .ListItemsSource(&TreeRootItems)
                        .ItemWidth(150)
                        .ItemHeight(150)
                        .SelectionMode(ESelectionMode::Single)
                        .OnGenerateTile(this, &UDoodleEffectLibraryWidget::MakeTableRowWidget)
                        .OnSelectionChanged_Lambda([&](TSharedPtr<FEffectTreeItem> inSelectItem, ESelectInfo::Type SelectType)
                        {
                            if (inSelectItem)
                            {
                                CurrentItem = inSelectItem;
                                TObjectPtr<UFileMediaSource> TheMediaSource = NewObject<UFileMediaSource>(GetTransientPackage());
                                TheMediaSource->SetFilePath(CurrentItem->PreviewFile);
                                CurrentItem->MediaPlayer->SetDesiredPlayerName(TEXT("VlcMedia"));
                                if (CurrentItem->MediaPlayer->CanPlaySource(TheMediaSource.Get())) 
                                {
                                    CurrentItem->MediaPlayer->OpenSource(TheMediaSource.Get());
                                }
                            }
                        })
                        .OnContextMenuOpening_Lambda([this]()
                        {
                            FUIAction ActionCall(FExecuteAction::CreateRaw(this, &UDoodleEffectLibraryWidget::OnEffectExport), FCanExecuteAction());
                            FMenuBuilder MenuBuilder(true, false);
                            MenuBuilder.AddMenuSeparator();
                            MenuBuilder.AddMenuEntry(FText::FromString(TEXT("导出")), FText::FromString(TEXT("导出到本地")),
                                FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Import")), ActionCall);
                            return MenuBuilder.MakeWidget();
                        })
                        .OnMouseButtonDoubleClick_Lambda([&](TSharedPtr<FEffectTreeItem> inSelectItem)
                        {
                            if (inSelectItem)
                            {
                                CurrentItem = inSelectItem;
                            }
                            OnPlayPreview(inSelectItem);
                        })
                ]
                +SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SDirectoryPicker)
                        .Directory(LibraryPath)
                        .OnDirectoryChanged(this, &UDoodleEffectLibraryWidget::OnDirectoryChanged)
                ]
        ];
}

TSharedRef<SDockTab> UDoodleEffectLibraryWidget::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) 
{
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(UDoodleEffectLibraryWidget)];
}

TSharedRef<ITableRow> UDoodleEffectLibraryWidget::MakeTableRowWidget(TSharedPtr<FEffectTreeItem> InTreeElement, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FEffectTreeItem>>, OwnerTable)
        [
            SNew(SBorder)
                .ToolTip(SNew(SToolTip).Text
                (
                    FText::FromString(InTreeElement->DescText))
                )
                .Padding(2)
                [
                    SNew(SOverlay)
                        + SOverlay::Slot()
                        .Padding(2)
                        [
                            SNew(SImage)
                                .Image(InTreeElement->ShotBrush.IsValid() ? InTreeElement->ShotBrush.Get() : nullptr)
                        ]
                        + SOverlay::Slot()
                        [
                            SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .VAlign(VAlign_Bottom)
                                .HAlign(HAlign_Left)
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromName(InTreeElement->Name))
                                        .Font(FStyleDefaults::GetFontInfo(10))
                                ]
                        ]
                        + SOverlay::Slot()
                        [
                            SAssignNew(InTreeElement->MediaBox, SBox)
                                .Visibility(EVisibility::Hidden)
                                [
                                    SNew(SMediaImage, InTreeElement->MediaTexture)
                                ]
                        ]
                ]
        ];
}

void UDoodleEffectLibraryWidget::OnCreateNewEffect()
{
    TSharedRef<SCreateEffectDialog> Dialog = SNew(SCreateEffectDialog);
    Dialog->Show();
    if (Dialog.Get().TheAssetData != nullptr)
    {
        SelectAssetData = Dialog.Get().TheAssetData;
        //----------------------
        TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->TryInvokeTab(UDoodleEffectLibraryEditWidget::Name);
        TSharedRef<UDoodleEffectLibraryEditWidget> Widget = StaticCastSharedRef<UDoodleEffectLibraryEditWidget>(Tab->GetContent());
        Widget->LibraryPath = LibraryPath;
        Widget->SelectObject = SelectAssetData.GetAsset();
        FName EffectName;
        if (Widget->SelectObject)
        {
            EffectName = SelectAssetData.AssetName;
        }
        else
        {
            EffectName = FName(TEXT("Effect"));
        }
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
        while (FileNames.Contains(EffectName))
        {
            int Counter = EffectName.GetNumber();
            EffectName.SetNumber(++Counter);
        }
        Widget->EffectName = EffectName.ToString();
        Widget->EffectTypeValues.Empty();
        for (TSharedPtr<FEffectTreeItem> Item : AllTreeRootItems)
        {
            if(Item->EffectType.Len()>0&& !Widget->EffectTypeValues.Contains(Item->EffectType))
            {
                Widget->EffectTypeValues.Add(Item->EffectType);
            }
        }
        if (!Widget->EffectTypeValues.IsEmpty())
            Widget->EffectType = Widget->EffectTypeValues.Top();
        Widget->SetViewportDat();
    }
}

TSharedRef<SWidget> UDoodleEffectLibraryWidget::MakeAddFilterMenu()
{
    FMenuBuilder MenuBuilder(true, nullptr, nullptr, true);
    MenuBuilder.BeginSection(TEXT("SectionExtensionHook"));
    {
        TArray<FString> EffectTypeValues;
        EffectTypeValues.Add(TEXT("全部"));
        for (TSharedPtr<FEffectTreeItem> Item : AllTreeRootItems)
        {
            if (Item->EffectType.Len() > 0 && !EffectTypeValues.Contains(Item->EffectType))
            {
                EffectTypeValues.Add(Item->EffectType);
            }
        }
        for (int32 i = 0; i < EffectTypeValues.Num(); i++)
        {
            MenuBuilder.AddMenuEntry(FText::FromString(EffectTypeValues[i]), TAttribute<FText>(), FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([this, EffectTypeValues, i]()
            {
                EffectType = EffectTypeValues[i];
                TreeRootItems.Empty();
                for (TSharedPtr<FEffectTreeItem> Item : AllTreeRootItems)
                {
                    if (EffectType == TEXT("全部")) 
                        TreeRootItems.Add(Item);
                    else
                    {
                        if (Item->EffectType == EffectType) 
                        {
                            TreeRootItems.Add(Item);
                        }
                    }
                }
                if (TileViewPtr.IsValid())
                {
                    TileViewPtr->RequestListRefresh();
                }
            }),
            FCanExecuteAction()),
            NAME_None,
            EUserInterfaceActionType::Button);
        }
    }
    MenuBuilder.EndSection(); 
//--------------------
    return
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        [
            MenuBuilder.MakeWidget()
        ];
}

void UDoodleEffectLibraryWidget::OnPlayPreview(TSharedPtr<FEffectTreeItem> inSelectItem)
{
    TSharedRef<SPlayPreviewDialog> Dialog = SNew(SPlayPreviewDialog);
    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(FText::FromString(TEXT("特效预览")))
        .ClientSize(FVector2D{ 800,700 })
        .SupportsMinimize(false)
        .SupportsMaximize(false);
    Window->SetContent(Dialog);
    FSlateApplication::Get().AddWindow(Window);
    Dialog->Show(inSelectItem->PreviewFile);
}

void UDoodleEffectLibraryWidget::OnSaveNewEffect(FString EffectName)
{
    FString PathName = FPaths::Combine(LibraryPath , EffectName);
    TSharedPtr<FEffectTreeItem> Item = CreateTreeItem(PathName);
    Item->ReadJson();
    AllTreeRootItems.Add(Item);
    TreeRootItems.Add(Item);
    if (TileViewPtr.IsValid()) 
    {
        TileViewPtr->RequestListRefresh();
    }
}

void UDoodleEffectLibraryWidget::OnDirectoryChanged(const FString& Directory)
{
    LibraryPath = Directory;
    GConfig->SetString(TEXT("DoodleEffectLibrary"), TEXT("EffectLibraryPath"), *LibraryPath, GEngineIni);
    //------------
    AllTreeRootItems.Empty();
    TreeRootItems.Empty();
    IFileManager::Get().IterateDirectory(*LibraryPath, [this](const TCHAR* PathName, bool bIsDir)
    {
        if (bIsDir)
        {
            TSharedPtr<FEffectTreeItem> Item = CreateTreeItem(PathName);
            AllTreeRootItems.Add(Item);
            TreeRootItems.Add(Item);
        }
        return true; // Continue
    });
    if (TileViewPtr.IsValid()) 
    {
        TileViewPtr->RequestListRefresh();
    }
}

void UDoodleEffectLibraryWidget::OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo)
{
    if (CommitInfo == ETextCommit::OnCleared)
    {
        FilterText = TEXT("");
    }
    else if(CommitInfo == ETextCommit::OnEnter)
    {
        FilterText = InSearchText.ToString();
        //---------------
        TreeRootItems.Empty();
        for (TSharedPtr<FEffectTreeItem> Item : AllTreeRootItems)
        {
            if (Item->Name.ToString().Contains(FilterText)) 
            {
                TreeRootItems.Add(Item);
            }
        }
        if (TileViewPtr.IsValid()) 
        {
            TileViewPtr->RequestListRefresh();
        }
    }
}

TSharedPtr<FEffectTreeItem> UDoodleEffectLibraryWidget::CreateTreeItem(FString PathName)
{
    FString EffectName = FPaths::GetBaseFilename(PathName);
    TSharedPtr<FEffectTreeItem> Item = MakeShareable(new FEffectTreeItem());
    Item->PreviewFile = FPaths::Combine(PathName, TEXT("Effect.avi"));
    Item->JsonFile = FPaths::Combine(PathName, TEXT("Data.json"));
    Item->Name = FName(EffectName);
    Item->SetThumbnail(FPaths::Combine(PathName, TEXT("Shot.png")));
    return Item;
}

void UDoodleEffectLibraryWidget::OnEffectExport()
{
    if (CurrentItem) 
    {
        FString DefaultPath = FPaths::ProjectContentDir();
        //---------------
        bool bFolderSelected = false;
        IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
        if (DesktopPlatform)
        {
            void* TopWindowHandle = FSlateApplication::Get().GetActiveTopLevelWindow().IsValid() ? FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle() : nullptr;
            FString FolderName;
            bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
                TopWindowHandle,
                TEXT("Message"),
                DefaultPath,
                FolderName
            );
            if (bFolderSelected)
            {
                ExportDirectory = FolderName;
                //--------------------------------
                FString ProjectContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
                FString SourcePath = FPaths::GetPath(CurrentItem->JsonFile);
                FString AbsoluteSrcDir = FPaths::ConvertRelativePathToFull(SourcePath);
                //----------------
                TArray<FString> FilesToCopy;
                IFileManager::Get().FindFilesRecursive(FilesToCopy, *AbsoluteSrcDir, TEXT("*"), true, true);
                ParallelFor(FilesToCopy.Num(), [&](int32 Index)
                {
                    const FString& SourceFilePath = FilesToCopy[Index];
                    if (FPaths::GetExtension(SourceFilePath, true) != TEXT(".json")
                        && FPaths::GetExtension(SourceFilePath, true) != TEXT(".png")
                        && FPaths::GetExtension(SourceFilePath, true) != TEXT(".avi"))
                    {
                        FString DestFilePath = FPaths::Combine(ProjectContentDir / CurrentItem->Name.ToString(), SourceFilePath.RightChop(AbsoluteSrcDir.Len()));
                        IFileManager::Get().Copy(*DestFilePath, *SourceFilePath, true, true);
                    }
                });
                //---------------------
                FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
                AssetRegistryModule.Get().ScanPathsSynchronous({ FPaths::ProjectContentDir() }, true);
                FString RelativePath = ExportDirectory.RightChop(ProjectContentDir.Len());
                UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
                EditorAssetSubsystem->RenameDirectory(FPaths::Combine(TEXT("/Game"), CurrentItem->Name.ToString()), FPaths::Combine(TEXT("/Game") / RelativePath, CurrentItem->Name.ToString()));
                //-----------------------
                FString Info = FString::Format(TEXT("导入特效：{0}到项目:{1}完成"), { CurrentItem->Name.ToString(),FPaths::Combine(TEXT("/Game"), RelativePath) });
                FNotificationInfo L_Info{ FText::FromString(Info) };
                L_Info.FadeInDuration = 1.0f;  // 
                L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
                FSlateNotificationManager::Get().AddNotification(L_Info);
            }
        }
    }
}