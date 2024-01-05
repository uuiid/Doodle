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
#include "Filters/SFilterBar.h"

#include "Materials/MaterialInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "NiagaraEmitter.h"
#include "NiagaraEditor/Public/NiagaraParameterDefinitions.h"
#include "NiagaraParameterCollection.h"

const FName UDoodleEffectLibraryWidget::Name{ TEXT("DoodleEffectLibraryWidget") };

class SPlayPreviewDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPlayPreviewDialog)
        {}
    SLATE_END_ARGS()
    //--------------
private:
    TObjectPtr<UFileMediaSource> MediaSource;
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
        MediaSource = NewObject<UFileMediaSource>(GetTransientPackage());
        MediaSource->AddToRoot();
        MediaPlayer = NewObject<UMediaPlayer>(GetTransientPackage(), NAME_None, RF_Transient);
        MediaPlayer->AddToRoot();
        MediaPlayer->SetLooping(true);
        //----------
        MediaTexture = NewObject<UMediaTexture>(GetTransientPackage(), NAME_None, RF_Transient);
        MediaTexture->AddToRoot();
        MediaTexture->SetDefaultMediaPlayer(MediaPlayer.Get());
        MediaTexture->AutoClear = true;
        MediaTexture->UpdateResource();
        MediaTexture->ClearColor = FLinearColor::Transparent;
    }

    ~SPlayPreviewDialog() 
    {
        if (MediaSource)
            MediaSource->RemoveFromRoot();
        if (MediaPlayer)
            MediaPlayer->RemoveFromRoot();
        if (MediaTexture)
            MediaTexture->RemoveFromRoot();
    }

    void Show(FString PreviewFile)
    {
        MediaSource->SetFilePath(PreviewFile);
        MediaPlayer->SetDesiredPlayerName(TEXT("VlcMedia"));
        if (MediaPlayer->CanPlaySource(MediaSource.Get()))
        {
            MediaPlayer->OpenSource(MediaSource.Get());
        }
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
    AllTileRootItems.Empty();
    TileRootItems.Empty();
    IFileManager::Get().IterateDirectory(*LibraryPath, [this](const TCHAR* PathName, bool bIsDir)
    {
        if (bIsDir)
        {
            TSharedPtr<FEffectTileItem> Item = CreateTileItem(PathName);
            AllTileRootItems.Add(Item);
            TileRootItems.Add(Item);
        }
        return true; // Continue
    });
    //Type--------------------
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
                LayerItem->ConvertToPath();
                LayerIndex++;
            }
        }
        return true;
    });
    //Tags------------------
    AllEffectTags.Empty();
    for (TSharedPtr<FEffectTileItem> Item : AllTileRootItems)
    {
        for (FString TempTag : Item->EffectTags)
        {
            if (TempTag.Len() > 0 && !AllEffectTags.Contains(TempTag))
            {
                AllEffectTags.Add(TempTag);
            }
        }
    }
    for (FString TempTag : AllEffectTags) 
    {
        TSharedPtr<FTagItem> TagItem = MakeShareable(new FTagItem());
        TagItem->Name = TempTag;
        TheEffectTags.Add(TagItem);
    };
}

UDoodleEffectLibraryWidget::~UDoodleEffectLibraryWidget() 
{
}

void UDoodleEffectLibraryWidget::Construct(const FArguments& InArgs)
{
    ChildSlot
        [
            SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(0.1)
                [
                    SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                                .Text(FText::FromString(TEXT("特效标签：")))
                                .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
                        ]
                        + SVerticalBox::Slot()
                        .Padding(2)
                        [
                            SAssignNew(EffectTagsViewPtr, SListView< TSharedPtr<FTagItem> >)
                                .SelectionMode(ESelectionMode::None)
                                .ItemHeight(40)
                                .ListItemsSource(&TheEffectTags)
                                .OnGenerateRow(this, &UDoodleEffectLibraryWidget::ListOnGenerateRow)
                        ]
                ]
                + SHorizontalBox::Slot()
                .FillWidth(0.8)
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
                        .AutoHeight()
                        [
                            SAssignNew(SearchBoxPtr, SAssetSearchBox)
                                .HintText(FText::FromString(TEXT("搜索特效名")))
                                .OnTextCommitted(this, &UDoodleEffectLibraryWidget::OnSearchBoxCommitted)
                                .DelayChangeNotificationsWhileTyping(true)
                                .AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ExtContentBrowserSearchAssets")))
                                .ShowSearchHistory(true)
                        ]
                        + SVerticalBox::Slot()
                        .Padding(2.0f)
                        [
                            SAssignNew(TileViewPtr, STileView<TSharedPtr<FEffectTileItem>>)
                                .ListItemsSource(&TileRootItems)
                                .ItemWidth(150)
                                .ItemHeight(160)
                                .SelectionMode(ESelectionMode::Single)
                                .OnGenerateTile(this, &UDoodleEffectLibraryWidget::MakeTableRowWidgetTile)
                                .OnSelectionChanged_Lambda([&](TSharedPtr<FEffectTileItem> inSelectItem, ESelectInfo::Type SelectType)
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
                                        .OnMouseButtonDoubleClick_Lambda([&](TSharedPtr<FEffectTileItem> inSelectItem)
                                            {
                                                if (inSelectItem)
                                                {
                                                    CurrentItem = inSelectItem;
                                                }
                                                OnPlayPreview(inSelectItem);
                                            })
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(SDirectoryPicker)
                                .Directory(LibraryPath)
                                .OnDirectoryChanged(this, &UDoodleEffectLibraryWidget::OnDirectoryChanged)
                        ]
                ]
                + SHorizontalBox::Slot()
                .FillWidth(0.2)
                [
                    SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                                .Text(FText::FromString(TEXT("特效分类：")))
                                .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
                        ]
                        + SVerticalBox::Slot()
                        .Padding(2)
                        [
                            SAssignNew(TreeViewPtr, STreeView<TSharedPtr<FTypeItem>>)
                                .TreeItemsSource(&RootChildren)
                                .SelectionMode(ESelectionMode::Single)
                                .OnGenerateRow(this, &UDoodleEffectLibraryWidget::MakeTableRowWidget)
                                .OnGetChildren(this, &UDoodleEffectLibraryWidget::HandleGetChildrenForTree)
                                .HighlightParentNodesForSelection(true)
                                .OnSelectionChanged(this, &UDoodleEffectLibraryWidget::OnTypeSelectionChanged)
                                .HeaderRow(
                                    SNew(SHeaderRow)
                                        + SHeaderRow::Column(FName(TEXT("Column1")))
                                        .Visibility(EVisibility::Hidden)
                                        .DefaultLabel(FText::FromString(TEXT("")))
                                )
                        ]
                ]
        ];
}

TSharedRef<SDockTab> UDoodleEffectLibraryWidget::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) 
{
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(UDoodleEffectLibraryWidget)];
}

TSharedRef<ITableRow> UDoodleEffectLibraryWidget::MakeTableRowWidgetTile(TSharedPtr<FEffectTileItem> InTreeElement, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FEffectTileItem>>, OwnerTable)
        [
            SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .FillHeight(0.9)
                [
                    SNew(SBorder)
                        .Padding(1)
                        [
                            SNew(SOverlay)
                                .ToolTip(SNew(SToolTip).Text
                                (
                                    FText::FromString(InTreeElement->DescText))
                                )
                                + SOverlay::Slot()
                                .Padding(2)
                                [
                                    SNew(SImage)
                                        .Image(InTreeElement->ShotBrush.IsValid() ? InTreeElement->ShotBrush.Get() : nullptr)
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
                ]
                + SVerticalBox::Slot()
                .HAlign(HAlign_Center)
                .FillHeight(0.1)
                [
                    SNew(STextBlock)
                        .Text(FText::FromName(InTreeElement->Name))
                        .Font(FStyleDefaults::GetFontInfo(10))
                ]
        ];
}

void UDoodleEffectLibraryWidget::OnPlayPreview(TSharedPtr<FEffectTileItem> inSelectItem)
{
    TSharedRef<SPlayPreviewDialog> Dialog = SNew(SPlayPreviewDialog);
    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(FText::FromString(TEXT("特效预览")))
        .ClientSize(FVector2D{ 1024,1024 })
        .SupportsMinimize(false)
        .SupportsMaximize(false);
    Window->SetContent(Dialog);
    FSlateApplication::Get().AddWindow(Window);
    Dialog->Show(inSelectItem->PreviewFile);
}

void UDoodleEffectLibraryWidget::OnSaveNewEffect(FString EffectName)
{
    FString PathName = FPaths::Combine(LibraryPath , EffectName);
    TSharedPtr<FEffectTileItem> Item = CreateTileItem(PathName);
    AllTileRootItems.Add(Item);
    TileRootItems.Add(Item);
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
    AllTileRootItems.Empty();
    TileRootItems.Empty();
    IFileManager::Get().IterateDirectory(*LibraryPath, [this](const TCHAR* PathName, bool bIsDir)
    {
        if (bIsDir)
        {
            TSharedPtr<FEffectTileItem> Item = CreateTileItem(PathName);
            AllTileRootItems.Add(Item);
            TileRootItems.Add(Item);
        }
        return true; // Continue
    });
    if (TileViewPtr.IsValid()) 
    {
        TileViewPtr->RequestListRefresh();
    }
}

TSharedPtr<FEffectTileItem> UDoodleEffectLibraryWidget::CreateTileItem(FString PathName)
{
    FString EffectName = FPaths::GetBaseFilename(PathName);
    TSharedPtr<FEffectTileItem> Item = MakeShareable(new FEffectTileItem());
    Item->PreviewFile = FPaths::Combine(PathName, TEXT("Effect.avi"));
    Item->JsonFile = FPaths::Combine(PathName, TEXT("Data.json"));
    Item->Name = FName(EffectName);
    Item->SetThumbnail(FPaths::Combine(PathName, TEXT("Shot.png")));
    Item->ReadJson();
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
                ////Move-----------------
                FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
                AssetRegistryModule.Get().ScanPathsSynchronous({ FPaths::ProjectContentDir() }, true);
                FString RelativePath = ExportDirectory.RightChop(ProjectContentDir.Len());
                UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
                FString TargetPath = FPaths::Combine(TEXT("/Game") / RelativePath, CurrentItem->Name.ToString());
                EditorAssetSubsystem->RenameDirectory(FPaths::Combine(TEXT("/Game"), CurrentItem->Name.ToString()), TargetPath);
                //-----------------------
                AssetRegistryModule.Get().ScanPathsSynchronous({ FPaths::ProjectContentDir() }, true);
                OnSortAssetPath(FName(TargetPath));
                FString Info = FString::Format(TEXT("导入特效：{0}到项目:{1}完成"), { CurrentItem->Name.ToString(),FPaths::Combine(TEXT("/Game"), RelativePath) });
                FNotificationInfo L_Info{ FText::FromString(Info) };
                L_Info.FadeInDuration = 1.0f;  // 
                L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
                FSlateNotificationManager::Get().AddNotification(L_Info);
            }
        }
    }
}

void UDoodleEffectLibraryWidget::OnSortAssetPath(FName AssetPath)
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

TSharedRef<ITableRow> UDoodleEffectLibraryWidget::MakeTableRowWidget(TSharedPtr<FTypeItem> InTreeElement, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(FTypeItemElement, OwnerTable, InTreeElement);
}

void UDoodleEffectLibraryWidget::HandleGetChildrenForTree(TSharedPtr<FTypeItem> InItem, TArray<TSharedPtr<FTypeItem>>& OutChildren)
{
    OutChildren.Append(InItem->Children);
}

TSharedRef<ITableRow> UDoodleEffectLibraryWidget::ListOnGenerateRow(TSharedPtr<FTagItem> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FTagItem>>, OwnerTable)
        [
            SNew(SBorder)
                .Padding(1,5)
                //.BorderImage(FAppStyle::Get().GetBrush("FilterBar.FilterBackground"))
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .VAlign(VAlign_Center)
                        .AutoWidth()
                        [
                            SNew(SImage)
                                .Image(FAppStyle::Get().GetBrush("FilterBar.FilterImage"))
                                .ColorAndOpacity_Lambda([InItem]()
                                {
                                    FLinearColor FilterColor = FLinearColor::Yellow;
                                    return InItem->IsChecked ? FilterColor : FAppStyle::Get().GetSlateColor("Colors.Recessed");
                                })
                        ]
                        + SHorizontalBox::Slot()
                        .Padding(TAttribute<FMargin>(InItem->IsChecked ? FMargin(4, 2, 4, 0) : FMargin(4, 1, 4, 1)))
                        .VAlign(VAlign_Center)
                        [
                            SAssignNew(ToggleButtonPtr, SFilterCheckBox)
                                .Style(FAppStyle::Get(), "FilterBar.FilterButton")
                                .IsChecked(InItem->IsChecked)
                                .OnCheckStateChanged(this, &UDoodleEffectLibraryWidget::OnTageCheckStateChanged, InItem)
                                .CheckBoxContentUsesAutoWidth(false)
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString(InItem->Name))
                                        .IsEnabled_Lambda([InItem] {return InItem->IsChecked; })
                                ]
                        ]
                ]
        ];
}

void UDoodleEffectLibraryWidget::OnTageCheckStateChanged(ECheckBoxState NewState,TSharedPtr<FTagItem> P_Item)
{
    P_Item->IsChecked = NewState == ECheckBoxState::Checked;
    FilterTags.Empty();
    for (TSharedPtr<FTagItem> TagItem : TheEffectTags)
    {
        if(TagItem->IsChecked)
            FilterTags.Add(TagItem->Name);
    }
    //--------------
    OnFilterTileView();
    if (TileViewPtr.IsValid())
    {
        TileViewPtr->RequestListRefresh();
    }
}

void UDoodleEffectLibraryWidget::OnTypeSelectionChanged(TSharedPtr<FTypeItem> inSelectItem, ESelectInfo::Type SelectType)
{
    NowSelectTypeItem = inSelectItem;
    //--------------
    OnFilterTileView();
    if (TileViewPtr.IsValid())
    {
        TileViewPtr->RequestListRefresh();
    }
}

void UDoodleEffectLibraryWidget::OnFilterTileView() 
{
    TileRootItems.Empty();
    for (TSharedPtr<FEffectTileItem> Item : AllTileRootItems)
    {
        bool IsFilter = true;
        for (FString TagStr : FilterTags)
        {
            if (!Item->EffectTags.Contains(TagStr))
            {
                IsFilter = false;
                break;
            }
        }
        if (IsFilter)
        {
            if (NowSelectTypeItem && !NowSelectTypeItem->TypePaths.IsEmpty())
            {
                if (!Item->TypePaths.IsEmpty())
                {
                    if (Item->MatchFilter(NowSelectTypeItem->TypePaths))
                    {
                        if (Item->Name.ToString().Contains(FilterText))
                        TileRootItems.Add(Item);
                    }
                }
            }
            else
            {
                if (Item->Name.ToString().Contains(FilterText))
                TileRootItems.Add(Item);
            }
        }
    }
}

void UDoodleEffectLibraryWidget::OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo)
{
    if (CommitInfo == ETextCommit::OnCleared)
    {
        FilterText = TEXT("");
    }
    else if (CommitInfo == ETextCommit::OnEnter)
    {
        FilterText = InSearchText.ToString();
        //---------------
        OnFilterTileView();
        if (TileViewPtr.IsValid())
        {
            TileViewPtr->RequestListRefresh();
        }
    }
}