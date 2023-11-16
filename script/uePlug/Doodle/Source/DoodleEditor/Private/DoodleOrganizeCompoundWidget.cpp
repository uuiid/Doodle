// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleOrganizeCompoundWidget.h"

#include "ContentBrowserModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "Particles/ParticleSystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AssetViewUtils.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ObjectTools.h"
#include "Doodle/ResizeTexture.h"

const FName UDoodleOrganizeCompoundWidget::Name{ TEXT("DoodleOrganizeCompoundWidget") };

TArray<TSharedPtr<FTreeItem>>& FTreeItem::GetChildren()
{
    return Children;
}

void STextureTreeItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, const TSharedPtr<FTreeItem> InTreeElement)
{
    WeakTreeElement = InTreeElement;

    FSuperRowType::FArguments SuperArgs = FSuperRowType::FArguments();
    SMultiColumnTableRow::Construct(SuperArgs, InOwnerTable);
}

TSharedRef<SWidget> STextureTreeItem::GenerateWidgetForColumn(const FName& ColumnName)
{
    if (ColumnName == FName(TEXT("Column1")))
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
            .HAlign(HAlign_Left)
            [
                SNew(SImage)
                    .Image(this, &STextureTreeItem::ShowImage)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Left)
            [
                SNew(STextBlock)
                    .Text(WeakTreeElement->Path.Len()>0? FText::FromString(WeakTreeElement->Path) : FText::FromName(WeakTreeElement->Name))
                    //.ToolTipText(this, &STextureTreeItem::GetToolTipText)
                    //.ColorAndOpacity(this, &STextureTreeItem::GetTextColor)
                    //.OnDoubleClicked(this, &STextureTreeItem::OnDoubleClicked)
            ];
    }
    else if (ColumnName == FName(TEXT("Column2")))
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .HAlign(HAlign_Right)
                [
                    SNew(SButton)
                        .Visibility_Lambda([&]() 
                        {
                            if (WeakTreeElement.Get()->OnClickedEvent.IsBound())
                                return EVisibility::Visible;
                            return EVisibility::Hidden;
                        })
                        .Text(FText::FromString(TEXT("指定")))
                        .OnClicked_Lambda([&]()
                        {
                            if (WeakTreeElement.Get()->OnClickedEvent.IsBound())
                                WeakTreeElement.Get()->OnClickedEvent.Execute(WeakTreeElement);
                            return FReply::Handled();
                        })
                ];
    }
    return SNullWidget::NullWidget;
}

const FSlateBrush* STextureTreeItem::ShowImage() const
{
    if (WeakTreeElement.IsValid())
    {
        if (WeakTreeElement->Brush != nullptr)
        {
            return WeakTreeElement->Brush.Get();
        }
    }
    return FAppStyle::GetBrush("Icons.Search");
}

void UDoodleOrganizeCompoundWidget::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                    .Text(FText::FromString(TEXT("自动整理选定文件到指定文件夹：")))
                    .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .VAlign(VAlign_Center)
                    .FillWidth(1.0f)
                    [
                        SNew(STextBlock)
                            .Text(FText::FromString(TEXT("目标文件夹")))
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(4.0f)
                    [
                        SNew(SEditableTextBox)
                            .Text_Lambda([this]()-> FText
                            {
                                GConfig->GetString(TEXT("DoodleOrganize"), TEXT("TargetFolderName"), TargetFolderName, GEngineIni);
                                return FText::FromString(TargetFolderName);
                            })
                            .OnTextChanged_Lambda([this](const FText& In_Text)
                            {
                                TargetFolderName = In_Text.ToString();
                            })
                            .OnTextCommitted_Lambda([this](const FText& In_Text, ETextCommit::Type)
                            {
                                TargetFolderName = In_Text.ToString();
                                GConfig->SetString(TEXT("DoodleOrganize"), TEXT("TargetFolderName"), *TargetFolderName, GEngineIni);
                            })
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(2.0f)
                    [
                        SNew(SButton)
                            .Text(FText::FromString(TEXT("整理")))
                            .OnClicked(this, &UDoodleOrganizeCompoundWidget::GenerateFolders)
                    ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SButton)
                    .Text(FText::FromString(TEXT("获取所有重复贴图")))
                    .OnClicked(this, &UDoodleOrganizeCompoundWidget::GetAllRepetitiveTexture)
            ]
            + SVerticalBox::Slot()
            .Padding(2.0f)
            [
                SAssignNew(TreeView, STreeView<TSharedPtr<FTreeItem>>)
                    .TreeItemsSource(&RootChildren)
                    .SelectionMode(ESelectionMode::Single)
                    .OnGenerateRow(this, &UDoodleOrganizeCompoundWidget::MakeTableRowWidget)
                    .OnGetChildren(this, &UDoodleOrganizeCompoundWidget::HandleGetChildrenForTree)
                    .HighlightParentNodesForSelection(true)
                    .OnMouseButtonDoubleClick_Lambda([&](TSharedPtr<FTreeItem> inSelectItem) 
                    {
                        TArray<UObject*> Objects;
                        Objects.Add(inSelectItem->Asset.GetAsset());
                        FContentBrowserModule& ContentBrowserModle = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
                        ContentBrowserModle.Get().SyncBrowserToAssets(Objects);
                    })
                    .HeaderRow
                    (
                        SNew(SHeaderRow)

                        + SHeaderRow::Column(FName(TEXT("Column1")))
                        .DefaultLabel(FText::FromString(TEXT("贴图路径")))
                        //.OnSort(FOnSortModeChanged::CreateSP(this, &SControlRigProfilingView::OnSortColumnHeader))
                        .FillWidth(0.8f)

                        + SHeaderRow::Column(FName(TEXT("Column2")))
                        //.OnSort(FOnSortModeChanged::CreateSP(this, &SControlRigProfilingView::OnSortColumnHeader))
                        .FixedWidth(90.0f)
                        .VAlignHeader(VAlign_Center)
                        .HeaderContent()
                        [
                            SNew(STextBlock)
                                .Text(FText::FromString(TEXT("")))
                                .ToolTipText(FText::FromString(TEXT("TotalMilisecondsIncTooltip")))
                        ]
                    )
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    [
                        SNew(SButton)
                            .Text(FText::FromString(TEXT("获取（材质球）引用的引擎内置贴图到本地目录")))
                            .OnClicked(this, &UDoodleOrganizeCompoundWidget::GetReferenceEngineTexture)
                    ]
                    +SHorizontalBox::Slot()
                    [
                        SNew(SButton)
                            .Text(FText::FromString(TEXT("重置所有贴图大小（2的幂次方）")))
                            .OnClicked(this, &UDoodleOrganizeCompoundWidget::OnResizeTextureSize)
                    ]
            ]
    ];
}

TSharedRef<SDockTab> UDoodleOrganizeCompoundWidget::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
	return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(UDoodleOrganizeCompoundWidget)];
}

FReply UDoodleOrganizeCompoundWidget::GenerateFolders()
{
    FContentBrowserModule& ContentModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    TArray<FAssetData> SelectedAsset;
    ContentModule.Get().GetSelectedAssets(SelectedAsset);
    if (SelectedAsset.Num() <= 0)
    {
        FText  DialogText = FText::FromString(TEXT("请先在内容浏览器中，选择一个文件。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    if (TargetFolderName.TrimEnd().IsEmpty())
    {
        FText  DialogText = FText::FromString(TEXT("目标文件夹名称不能为空。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    FString ContentPath = TEXT("/Game/");
    FString FolderPath = FPaths::Combine(ContentPath, TargetFolderName.TrimEnd());
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    if (!EditorAssetSubsystem->DoesDirectoryExist(FolderPath))
    {
        EditorAssetSubsystem->MakeDirectory(FolderPath);
    }
    for (FAssetData SelectedData : SelectedAsset)
    {
        if (SelectedData.GetClass()->IsChildOf<UFXSystemAsset>())
        {
            FString Path = FPaths::Combine(FolderPath, TEXT("Fx"));
            if (!EditorAssetSubsystem->DoesDirectoryExist(Path))
            {
                EditorAssetSubsystem->MakeDirectory(Path);
            }
            AssetViewUtils::MoveAssets({ SelectedData.GetAsset() }, Path, SelectedData.PackagePath.ToString());
        }
        if (SelectedData.GetClass()->IsChildOf<UStaticMesh>())
        {
            FString Path = FPaths::Combine(FolderPath, TEXT("Mesh"));
            if (!EditorAssetSubsystem->DoesDirectoryExist(Path))
            {
                EditorAssetSubsystem->MakeDirectory(Path);
            }
            AssetViewUtils::MoveAssets({ SelectedData.GetAsset() }, Path, SelectedData.PackagePath.ToString());
        }
        if (SelectedData.GetClass()->IsChildOf<UTexture>())
        {
            FString Path = FPaths::Combine(FolderPath, TEXT("Tex"));
            if (!EditorAssetSubsystem->DoesDirectoryExist(Path))
            {
                EditorAssetSubsystem->MakeDirectory(Path);
            }
            AssetViewUtils::MoveAssets({ SelectedData.GetAsset() }, Path, SelectedData.PackagePath.ToString());
        }
        if (SelectedData.GetClass()->IsChildOf<UMaterial>())
        {
            FString Path = FPaths::Combine(FolderPath, TEXT("Mat"));
            if (!EditorAssetSubsystem->DoesDirectoryExist(Path))
            {
                EditorAssetSubsystem->MakeDirectory(Path);
            }
            AssetViewUtils::MoveAssets({ SelectedData.GetAsset() }, Path, SelectedData.PackagePath.ToString());
        }
        if (SelectedData.GetClass()->IsChildOf<UWorld>())
        {
            FString Path = FPaths::Combine(FolderPath, TEXT("Maps"));
            if (!EditorAssetSubsystem->DoesDirectoryExist(Path))
            {
                EditorAssetSubsystem->MakeDirectory(Path);
            }
            AssetViewUtils::MoveAssets({ SelectedData.GetAsset() }, Path, SelectedData.PackagePath.ToString());
        }
        if (SelectedData.GetClass()->IsChildOf<UBlueprint>())
        {
            FString Path = FPaths::Combine(FolderPath, TEXT("Blue"));
            if (!EditorAssetSubsystem->DoesDirectoryExist(Path))
            {
                EditorAssetSubsystem->MakeDirectory(Path);
            }
            AssetViewUtils::MoveAssets({ SelectedData.GetAsset() }, Path, SelectedData.PackagePath.ToString());
        }
    }
    //--------------------------
    return FReply::Handled();
}

FReply UDoodleOrganizeCompoundWidget::GetAllRepetitiveTexture()
{
    TArray<FAssetData> AllTexture;
    RootChildren.Empty();
    FARFilter LFilter{};
    LFilter.bRecursivePaths = true;
    LFilter.bRecursiveClasses = true;
    LFilter.PackagePaths.Add(FName{ TEXT("/Game") });
    LFilter.ClassPaths.Add(UTexture::StaticClass()->GetClassPathName());
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().GetAssets(LFilter, AllTexture);
    while (AllTexture.Num() > 0)
    {
        FAssetData asset = AllTexture.Top();
        //const FAssetPackageData* FoundData = AssetRegistryModule.Get().GetAssetPackageData(asset.PackageName);
        TArray<FAssetData> RepeatTexture;
        for (const FAssetData& AssetData : AllTexture)
        {
            //const FAssetPackageData* FoundData2 = AssetRegistryModule.Get().GetAssetPackageData(AssetData.PackageName);
            if (AssetData.GetClass()->GetName() == asset.GetClass()->GetName() && AssetData.AssetName == asset.AssetName
                && AssetData.PackagePath != asset.PackagePath)
            {
                RepeatTexture.Add(AssetData);
            }
        }
        if (RepeatTexture.Num() > 0)
        {
            RepeatTexture.Add(asset);
            for (FAssetData Texture : RepeatTexture)
            {
                TSharedPtr<FTreeItem> root = nullptr;
                for (TSharedPtr<FTreeItem> child : RootChildren)
                {
                    if (child->Name == Texture.AssetName)
                    {
                        root = child;
                        break;
                    }
                }
                if (!root) 
                {
                    root = MakeShareable(new FTreeItem());
                    root.Get()->Name = Texture.AssetName;
                    RootChildren.Add(root);
                }
                TSharedPtr<FTreeItem> Item = MakeShareable(new FTreeItem());
                Item.Get()->Path = Texture.PackageName.ToString();
                Item->Asset = Texture;
                Item->parent = root;
                TSharedPtr<FSlateBrush> TheBrush = MakeShareable(new FSlateBrush());
                TheBrush->SetResourceObject(Texture.GetAsset());
                TheBrush->ImageSize = FVector2D(32, 32);
                TheBrush->ImageType = ESlateBrushImageType::Linear;
                TheBrush->DrawAs = ESlateBrushDrawType::Image;
                Item->Brush = TheBrush;
                Item->OnClickedEvent.BindRaw(this, &UDoodleOrganizeCompoundWidget::OnAssginRepeatTexture);
                root->Children.Add(Item);
                AllTexture.Remove(Texture);
            }
        }
        else
        {
            AllTexture.Remove(asset);
        }
    }
    TreeView->RequestTreeRefresh();
    return FReply::Handled();
}

TSharedRef<ITableRow> UDoodleOrganizeCompoundWidget::MakeTableRowWidget(TSharedPtr<FTreeItem> InTreeElement, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STextureTreeItem, OwnerTable, InTreeElement);
}

void UDoodleOrganizeCompoundWidget::HandleGetChildrenForTree(TSharedPtr<FTreeItem> InItem, TArray<TSharedPtr<FTreeItem>>& OutChildren)
{
    OutChildren.Append(InItem->GetChildren());
    //SortChildren(OutChildren);
}

void UDoodleOrganizeCompoundWidget::OnAssginRepeatTexture(TSharedPtr<FTreeItem> Item)
{
    TArray<FString> ReferencerPath;
    TArray<FString> AssetPath;
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    //--------------------
    TArray<UObject*> FinalConsolidationObjects;
    for (TSharedPtr<FTreeItem> Child : Item->parent->Children)
    {
        TArray<FAssetDependency> Dependencys;
        AssetRegistryModule.Get().GetReferencers(Child->Asset.PackageName, Dependencys);
        if (!AssetPath.Contains(Child->Asset.PackagePath.ToString()))
        {
            AssetPath.Add(Child->Asset.PackagePath.ToString());
        }
        for (FAssetDependency Dependency : Dependencys)
        {
            Dependency.AssetId.PackageName;
            FString PackagePath = FPackageName::GetLongPackagePath(Dependency.AssetId.PackageName.ToString());
            if (!ReferencerPath.Contains(PackagePath))
            {
                ReferencerPath.Add(PackagePath);
            }
        }
        //-------------------
        FinalConsolidationObjects.Add(Child->Asset.GetAsset());
    }
    FinalConsolidationObjects.RemoveSingle(Item->Asset.GetAsset());
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    bool ConsResults = EditorAssetSubsystem->ConsolidateAssets(Item->Asset.GetAsset(), FinalConsolidationObjects);
    if (ConsResults)
    {
        TArray<TSharedPtr<FTreeItem>> RemoveList;
        for (TSharedPtr<FTreeItem> Child : Item->parent->Children)
        {
            EditorAssetSubsystem->SaveLoadedAsset(Child->Asset.GetAsset());
            if (Child != Item)
            {
                Child->Brush = nullptr;
                Child->OnClickedEvent = nullptr;
                Child->Path = TEXT("");
                RemoveList.Add(Child);
            }
        }
        for (TSharedPtr<FTreeItem> Remove : RemoveList)
        {
            Item->parent->Children.Remove(Remove);
        }
        //------------------------------------------
        TArray<UObject*> Objects;
        if (AssetViewUtils::LoadAssetsIfNeeded(ReferencerPath, Objects, true, true))
        {
            TArray<UObjectRedirector*> Redirectors;
            for (UObject* Object : Objects)
            {
                if (Object->GetClass()->IsChildOf(UObjectRedirector::StaticClass()))
                    Redirectors.Add(CastChecked<UObjectRedirector>(Object));
            }
            FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
            AssetToolsModule.Get().FixupReferencers(Redirectors);
        }
        for (FString Path : AssetPath)
        {
            TArray<FAssetData> AssetDatas;
            AssetRegistryModule.Get().GetAssetsByPath(FName(Path), AssetDatas, true);
            if (AssetDatas.Num() <=0)
            {
                EditorAssetSubsystem->DeleteDirectory(Path);
            }
        }
        //----------------------------------------
        FNotificationInfo L_Info{ FText::FromString(TEXT("指定成功")) };
        L_Info.FadeInDuration = 2.0f;  // 
        L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Info"));
        FSlateNotificationManager::Get().AddNotification(L_Info);
        //-----------------------------------
        //GetAllRepetitiveTexture();
    }
    TreeView->RequestTreeRefresh();
}

FReply UDoodleOrganizeCompoundWidget::GetReferenceEngineTexture()
{
    if (TargetFolderName.TrimEnd().IsEmpty())
    {
        FText  DialogText = FText::FromString(TEXT("目标文件夹名称不能为空。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    //------------
    FARFilter LFilter{};
    LFilter.bRecursivePaths = true;
    LFilter.bRecursiveClasses = true;
    LFilter.PackagePaths.Add(FName{ GamePath });
    LFilter.ClassPaths.Add(UMaterialInterface::StaticClass()->GetClassPathName());
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AllAsset;
    AssetRegistryModule.Get().GetAssets(LFilter,AllAsset);
    for (FAssetData Selected : AllAsset)
    {
        //--------------
        TArray<FAssetDependency> Dependencys;
        AssetRegistryModule.Get().GetDependencies(Selected.PackageName, Dependencys);
        for (FAssetDependency Dependency : Dependencys)
        {
            UObject* SourceObj = LoadObject<UObject>(nullptr, *Dependency.AssetId.PackageName.ToString());
            if (SourceObj&&SourceObj->GetClass()->IsChildOf(UTexture::StaticClass())) 
            {
                if (!Dependency.AssetId.PackageName.ToString().StartsWith(GamePath))
                {
                    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
                    ///-------------------------
                    FString FolderPath = FPaths::Combine(GamePath, TargetFolderName.TrimEnd());
                    FString Path = FPaths::Combine(FolderPath, TexturePath);
                    FString FileName = FPaths::GetBaseFilename(Dependency.AssetId.PackageName.ToString(), true);
                    FString FilePath = FPaths::Combine(Path, FileName);
                    if (!EditorAssetSubsystem->DoesAssetExist(FilePath))
                    {
                        FString backUpPath = Dependency.AssetId.PackageName.ToString() + TEXT("_bu");
                        if (!EditorAssetSubsystem->DoesAssetExist(backUpPath))
                        {
                            UObject* Obj = EditorAssetSubsystem->DuplicateAsset(Dependency.AssetId.PackageName.ToString(), backUpPath);
                            EditorAssetSubsystem->SaveLoadedAsset(Obj);
                        }
                        //-----------------
                        UObject* Obj = EditorAssetSubsystem->DuplicateAsset(Dependency.AssetId.PackageName.ToString(), FilePath);
                        if (Obj)
                        {
                            TArray<UObject*> ObjectsToReplace(&SourceObj, 1);
                            bool ConsResults = EditorAssetSubsystem->ConsolidateAssets(Obj, ObjectsToReplace);
                            if (ConsResults)
                            {
                                EditorAssetSubsystem->SaveLoadedAsset(Obj);
                                //EditorAssetSubsystem->SaveLoadedAsset(SourceObj);
                                //---------------------------------------
                                TArray<UObject*> Objects;
                                TArray<FString> SelectedAssetPaths;
                                SelectedAssetPaths.Add(Selected.GetObjectPathString());
                                if (AssetViewUtils::LoadAssetsIfNeeded(SelectedAssetPaths, Objects, true, true))
                                {
                                    TArray<UObjectRedirector*> Redirectors;
                                    for (UObject* Object : Objects)
                                    {
                                        if(Object->GetClass()->IsChildOf(UObjectRedirector::StaticClass()))
                                            Redirectors.Add(CastChecked<UObjectRedirector>(Object));
                                    }
                                    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
                                    AssetToolsModule.Get().FixupReferencers(Redirectors);
                                }
                                //-----------------------
                                FString Info = FString::Format(TEXT("复制贴图{0}到{1}成功"), { Dependency.AssetId.PackageName.ToString(), FilePath });
                                FNotificationInfo L_Info{ FText::FromString(Info) };
                                L_Info.FadeInDuration = 2.0f;  // 
                                L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
                                FSlateNotificationManager::Get().AddNotification(L_Info);
                            }
                        }
                    }
                }
            }
        }
    };
    return FReply::Handled();
}

FReply UDoodleOrganizeCompoundWidget::OnResizeTextureSize() 
{
    //-------------------
    FARFilter LFilter{};
    LFilter.bRecursivePaths = true;
    LFilter.bRecursiveClasses = true;
    LFilter.PackagePaths.Add(FName{ GamePath });
    LFilter.ClassPaths.Add(UTexture::StaticClass()->GetClassPathName());
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AllAsset;
    AssetRegistryModule.Get().GetAssets(LFilter, AllAsset);
    for (FAssetData Selected : AllAsset)
    {
        UTexture* In_Texture = Cast<UTexture>(Selected.GetAsset());
        FTextureSource& L_Soure = In_Texture->Source;
        //--------------------
        if(L_Soure.IsValid())
        {
            int32 L_Max_Size_Count = FMath::Max(FMath::CeilLogTwo(L_Soure.GetSizeX()), FMath::CeilLogTwo(L_Soure.GetSizeY()));//----------
            int32 L_Max_Size = FMath::Pow(2.0, L_Max_Size_Count);
            if (L_Max_Size != L_Soure.GetSizeX() || L_Max_Size != L_Soure.GetSizeY())
            {
                if (L_Soure.GetNumSlices() == 1 && L_Soure.GetNumBlocks() == 1 && !L_Soure.IsHDR(L_Soure.GetFormat()))
                {
                    FImage L_Image{};
                    switch (L_Soure.GetFormat())
                    {
                        case ETextureSourceFormat::TSF_G8: 
                        {
                            L_Image.Init(L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::G8);
                        } break;
                        case ETextureSourceFormat::TSF_BGRA8: 
                        {
                            L_Image.Init(L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::BGRA8);
                        } break;
                        case ETextureSourceFormat::TSF_BGRE8: 
                        {
                            L_Image.Init(L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::BGRE8);
                        } break;
                        case ETextureSourceFormat::TSF_RGBA16: 
                        {
                            L_Image.Init(L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::RGBA16);
                        } break;
                        case ETextureSourceFormat::TSF_RGBA16F: 
                        {
                            L_Image.Init(L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::RGBA16F);
                        } break;
                        case ETextureSourceFormat::TSF_G16: 
                        {
                            L_Image.Init(L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::G16);
                        } break;
                        default:
                            return FReply::Handled();
                            break;
                    }
                    ///-------------------------
                    In_Texture->ReleaseResource();
                    L_Soure.GetMipData(L_Image.RawData, 0);
                    //-----------------
                    FImage L_Dest;
                    L_Image.ResizeTo(L_Dest, L_Max_Size, L_Max_Size, L_Image.Format, L_Image.GammaSpace);
                    L_Soure.Init(L_Max_Size, L_Max_Size, 1, 1, L_Soure.GetFormat(), L_Dest.RawData.GetData());
                    In_Texture->MipGenSettings = TextureMipGenSettings::TMGS_FromTextureGroup;
                    //--------------
                    In_Texture->MarkPackageDirty();
                    In_Texture->PostEditChange();
                    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
                    EditorAssetSubsystem->SaveLoadedAsset(In_Texture);
                    //-----------------------
                    FString Info = FString::Format(TEXT("重置贴图{0}成功"), { Selected.PackageName.ToString() });
                    FNotificationInfo L_Info{ FText::FromString(Info) };
                    L_Info.FadeInDuration = 2.0f;  // 
                    L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
                    FSlateNotificationManager::Get().AddNotification(L_Info);
                }
            }
        }
    }
    return FReply::Handled();
}