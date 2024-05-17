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
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "HAL/FileManager.h"

const FName UDoodleOrganizeCompoundWidget::Name{ TEXT("DoodleOrganizeCompoundWidget") };

TArray<TSharedPtr<FTreeItem>>& FTreeItem::GetChildren()
{
    return Children;
}

void STextureTreeItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, const TSharedPtr<FTreeItem> InTreeElement)
{
    OnTextCommittedEvent = InArgs._OnTextCommittedEvent;
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
                SAssignNew(EditableText,SEditableText)
                    .MinDesiredWidth(400)
                    .IsEnabled(false)
                    .Text(WeakTreeElement->Path.Len()>0? FText::FromString(WeakTreeElement->Path) : FText::FromName(WeakTreeElement->Name))
                    .OnTextCommitted_Lambda([this](const FText& InText, const ETextCommit::Type InTextAction)
                    {
                        EditableText->SetText(FText::FromString(WeakTreeElement->Path));
                        EditableText->SetEnabled(false);
                        if (InText.ToString().Len() > 0) 
                        {
                            UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
                            FString NewPath = FPaths::Combine(WeakTreeElement->Asset.PackagePath.ToString(), InText.ToString());
                            if(!EditorAssetSubsystem->DoesAssetExist(NewPath))
                            {
                                if (EditorAssetSubsystem->RenameAsset(WeakTreeElement->Asset.PackageName.ToString(), NewPath))
                                {
                                    //-----------------------
                                    FString Info = FString::Format(TEXT("重命名为{0}成功"), { InText.ToString() });
                                    FNotificationInfo L_Info{ FText::FromString(Info) };
                                    L_Info.FadeInDuration = 2.0f;  // 
                                    L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
                                    FSlateNotificationManager::Get().AddNotification(L_Info);
                                    //---------------------------
                                    OnTextCommittedEvent.ExecuteIfBound(WeakTreeElement);
                                }
                            }
                            else
                            {
                                //-----------------------
                                FString Info = FString::Format(TEXT("重命名为{0}失败，该命名文件已存在"), { InText.ToString() });
                                FNotificationInfo L_Info{ FText::FromString(Info) };
                                L_Info.FadeInDuration = 2.0f;  // 
                                L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Error"));
                                FSlateNotificationManager::Get().AddNotification(L_Info);
                            }
                        }
                    })
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
                    .Text(FText::FromString(TEXT("部门：地编-----------------------------------------------------")))
                    .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
            ]
            + SVerticalBox::Slot()
            .FillHeight(0.1)
            [
                SNew(SHorizontalBox)
                    +SHorizontalBox::Slot()
                    [
                        SNew(SButton)
                            .Text(FText::FromString(TEXT("查找所有重复贴图")))
                            .OnClicked(this, &UDoodleOrganizeCompoundWidget::GetAllRepetitiveTexture)
                    ]
                    + SHorizontalBox::Slot()
                    [
                        SNew(SButton)
                            .Text(FText::FromString(TEXT("一键删除重复贴图")))
                            .OnClicked(this, &UDoodleOrganizeCompoundWidget::OneTouchDeleteRepectTexture)
                    ]
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
                    .OnSelectionChanged_Lambda([&](TSharedPtr<FTreeItem> inSelectItem, ESelectInfo::Type SelectType)
                    {
                        NowSelectItem = inSelectItem;
                    })
                    .OnContextMenuOpening_Lambda([this]()
                    {
                        if (NowSelectItem && NowSelectItem->Path.Len() > 0)
                        {
                            FUIAction ActionDelete(FExecuteAction::CreateRaw(this, &UDoodleOrganizeCompoundWidget::OnRenameTexture), FCanExecuteAction());
                            FMenuBuilder MenuBuilder(true, false);
                            MenuBuilder.AddMenuSeparator();
                            MenuBuilder.AddMenuEntry(FText::FromString(TEXT("重命名")), FText::FromString(TEXT("重命名贴图")),
                                FSlateIcon(), ActionDelete);
                            return MenuBuilder.MakeWidget();
                        }
                        return SNullWidget::NullWidget;
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
            [
                SNew(STextBlock)
                    .Text(FText::FromString(TEXT("自动整理选定文件到指定文件夹：")))
                    .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
            ]
            + SVerticalBox::Slot()
            .FillHeight(0.1)
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
                            .Text(FText::FromString(TEXT("整理选中资源")))
                            .OnClicked(this, &UDoodleOrganizeCompoundWidget::GenerateFolders)
                    ]
            ]
          
            + SVerticalBox::Slot()
            .FillHeight(0.1)
            .Padding(2)
            [
                SNew(SButton)
                    .Text(FText::FromString(TEXT("删除所有空文件夹")))
                    .OnClicked_Lambda([this]() 
                    {
                        DeleteAllEmptyDirectory();
                        return FReply::Handled();
                    })
            ]
            + SVerticalBox::Slot()
            .FillHeight(0.1)
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
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                    .Text(FText::FromString(TEXT("部门：角色模型-----------------------------------------------------")))
                    .ColorAndOpacity(FSlateColor{ FLinearColor{1, 1, 0, 1} })
            ]
            + SVerticalBox::Slot()
            .FillHeight(0.1)
            [
                SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .VAlign(VAlign_Center)
                    .FillWidth(1.0f)
                    [
                        SNew(STextBlock)
                            .Text(FText::FromString(TEXT("角色名称（拼音）:"+ ModeFolderName)))
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(4.0f)
                    [
                        SNew(SEditableTextBox)
                            .Text_Lambda([this]()-> FText
                            {
                                GConfig->GetString(TEXT("DoodleOrganize"), TEXT("ModeFolderName"), ModeFolderName, GEngineIni);
                                return FText::FromString(ModeFolderName);
                            })
                            .OnTextChanged_Lambda([this](const FText& In_Text)
                            {
                                ModeFolderName = In_Text.ToString();
                            })
                            .OnTextCommitted_Lambda([this](const FText& In_Text, ETextCommit::Type)
                            {
                                ModeFolderName = In_Text.ToString();
                                GConfig->SetString(TEXT("DoodleOrganize"), TEXT("ModeFolderName"), *ModeFolderName, GEngineIni);
                            })
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(2.0f)
                    [
                        SNew(SButton)
                            .Text(FText::FromString(TEXT("整理所有资源")))
                            .OnClicked(this, &UDoodleOrganizeCompoundWidget::GenerateModeFolders)
                    ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(0.1)
            .Padding(5.0f)
            [
                SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(SButton)
                            .Text(FText::FromString(TEXT("去除文件_后缀")))
                            .OnClicked(this, &UDoodleOrganizeCompoundWidget::RemoveSuffix)
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(4.0f)
                    [
                        SNew(SEditableTextBox)
                            .Text_Lambda([this]()-> FText
                            {
                                GConfig->GetString(TEXT("DoodleOrganize"), TEXT("TheSuffix"), TheSuffix, GEngineIni);
                                return FText::FromString(TheSuffix);
                            })
                            .OnTextChanged_Lambda([this](const FText& In_Text)
                            {
                                TheSuffix = In_Text.ToString();
                            })
                            .OnTextCommitted_Lambda([this](const FText& In_Text, ETextCommit::Type)
                            {
                                TheSuffix = In_Text.ToString();
                                GConfig->SetString(TEXT("DoodleOrganize"), TEXT("TheSuffix"), *TheSuffix, GEngineIni);
                            })
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(2.0f)
                    [
                        SNew(SButton)
                            .Text(FText::FromString(TEXT("添加文件_后缀")))
                            .OnClicked(this, &UDoodleOrganizeCompoundWidget::AddSuffix)
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
    TArray<UObject*> SelectedObjs;
    for (FAssetData SelectedData : SelectedAsset)
    {
        if (SelectedData.GetClass()->IsChildOf<UFXSystemAsset>())
        {
            MakeDirectoryByType(FolderPath, SelectedData, TEXT("Fx"));
        }
        else if (SelectedData.GetClass()->IsChildOf<UStaticMesh>())
        {
            MakeDirectoryByType(FolderPath, SelectedData, TEXT("Mesh"));
        }
        else if (SelectedData.GetClass()->IsChildOf<UTexture>())
        {
            MakeDirectoryByType(FolderPath, SelectedData, TEXT("Tex"));
        }
        else if (SelectedData.GetClass() == UMaterialInstance::StaticClass())
        {
            MakeDirectoryByType(FolderPath, SelectedData, TEXT("Mat/MatInst"));
        }
        else if (SelectedData.GetClass() == UMaterial::StaticClass())
        {
            MakeDirectoryByType(FolderPath, SelectedData, TEXT("Mat/Mat"));
        }
        else if (SelectedData.GetClass() == UMaterialParameterCollection::StaticClass())
        {
            MakeDirectoryByType(FolderPath, SelectedData, TEXT("Mat/MatParSet"));
        }
        else if (SelectedData.GetClass() == UMaterialFunction::StaticClass())
        {
            MakeDirectoryByType(FolderPath, SelectedData, TEXT("Mat/MatFun"));
        }
        else if (SelectedData.GetClass()->IsChildOf<UWorld>())
        {
            MakeDirectoryByType(FolderPath, SelectedData, TEXT("Maps"));
        }
        else if (SelectedData.GetClass()->IsChildOf<UBlueprint>())
        {
            MakeDirectoryByType(FolderPath, SelectedData, TEXT("Blue"));
        }
        else
        {
            MakeDirectoryByType(FolderPath,SelectedData, TEXT("Other"));
        }
        SelectedObjs.Add(SelectedData.GetAsset());
    }
    //--------------------------
    EditorAssetSubsystem->SaveLoadedAssets(SelectedObjs);
    //------------
    FixupAllReferencers();
    return FReply::Handled();
}

void UDoodleOrganizeCompoundWidget::MakeDirectoryByType(FString FolderPath,FAssetData SelectedData,FString DirectoryName)
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    FString Path = FPaths::Combine(FolderPath, DirectoryName);
    if (!EditorAssetSubsystem->DoesDirectoryExist(Path))
    {
        EditorAssetSubsystem->MakeDirectory(Path);
    }
    FName AssetName = SelectedData.AssetName;
    if (!EditorAssetSubsystem->DoesAssetExist(FPaths::Combine(Path, AssetName.ToString())))
    {
        AssetViewUtils::MoveAssets({ SelectedData.GetAsset() }, Path, SelectedData.PackagePath.ToString());
    }
    else
    {
        while (EditorAssetSubsystem->DoesAssetExist(FPaths::Combine(Path, AssetName.ToString())))
        {
            int Counter = AssetName.GetNumber();
            AssetName.SetNumber(++Counter);
        }
        if (EditorAssetSubsystem->RenameAsset(SelectedData.PackageName.ToString(), FPaths::Combine(Path, AssetName.ToString())))
        {
            FString Info = FString::Format(TEXT("文件重命名为:{0}"), { FPaths::Combine(SelectedData.PackagePath.ToString(), AssetName.ToString()) });
            FNotificationInfo L_Info{ FText::FromString(Info) };
            L_Info.FadeInDuration = 2.0f;  // 
            L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
            FSlateNotificationManager::Get().AddNotification(L_Info);
        }
    }
    //EditorAssetSubsystem->SaveLoadedAsset(SelectedData.GetAsset());
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
    return SNew(STextureTreeItem, OwnerTable, InTreeElement)
        .OnTextCommittedEvent_Lambda([this](TSharedPtr<FTreeItem> InItem)
        {
            InItem->parent->Children.Remove(InItem);
            TreeView->RequestTreeRefresh();
        });
}

void UDoodleOrganizeCompoundWidget::HandleGetChildrenForTree(TSharedPtr<FTreeItem> InItem, TArray<TSharedPtr<FTreeItem>>& OutChildren)
{
    OutChildren.Append(InItem->GetChildren());
    //SortChildren(OutChildren);
}

void UDoodleOrganizeCompoundWidget::OnAssginRepeatTexture(TSharedPtr<FTreeItem> Item)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    //--------------------
    TArray<UObject*> FinalConsolidationObjects;
    for (TSharedPtr<FTreeItem> Child : Item->parent->Children)
    {
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
        //------------------
        FixupAllReferencers();
        //----------------------------------------
        FNotificationInfo L_Info{ FText::FromString(TEXT("指定成功")) };
        L_Info.FadeInDuration = 2.0f;  // 
        L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
        FSlateNotificationManager::Get().AddNotification(L_Info);
        //-----------------------------------
    }
    TreeView->RequestTreeRefresh();
}

void UDoodleOrganizeCompoundWidget::FixupAllReferencers()
{
    FARFilter LFilter{};
    LFilter.bRecursivePaths = true;
    LFilter.bRecursiveClasses = true;
    LFilter.PackagePaths.Add(FName{ GamePath });
    LFilter.ClassPaths.Add(UObjectRedirector::StaticClass()->GetClassPathName());
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AllAsset;
    AssetRegistryModule.Get().GetAssets(LFilter, AllAsset);
    TArray<FString> SelectedAssetPaths;
    TArray<UObject*> Objects;
    for (FAssetData Selected : AllAsset) 
    {
        if(!SelectedAssetPaths.Contains(Selected.GetObjectPathString()))
            SelectedAssetPaths.Add(Selected.GetObjectPathString());
    }
    if (AssetViewUtils::LoadAssetsIfNeeded(SelectedAssetPaths, Objects, true, true))
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
    FixupAllReferencers();
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
        if(In_Texture &&L_Soure.IsValid())
        {
            int32 L_Max_Size_Count = FMath::Max(FMath::CeilLogTwo(L_Soure.GetSizeX()), FMath::CeilLogTwo(L_Soure.GetSizeY()));//----------
            int32 L_Max_Size = FMath::Pow(2.0, L_Max_Size_Count);
            if (L_Max_Size != L_Soure.GetSizeX() || L_Max_Size != L_Soure.GetSizeY())
            {
                FResizeTexture L_Resize{};
                L_Resize.Resize(In_Texture);
                //----------------------
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
    return FReply::Handled();
}

void UDoodleOrganizeCompoundWidget::OnRenameTexture()
{
    if (NowSelectItem) 
    {
        TSharedPtr<ITableRow> TableRow = TreeView->WidgetFromItem(NowSelectItem);
        TSharedPtr <STextureTreeItem> Row = StaticCastSharedPtr<STextureTreeItem>(TableRow);
        if (Row.IsValid())
        {
            Row->EditableText->SetText(FText::FromName(NowSelectItem->Asset.AssetName));
            Row->EditableText->SetEnabled(true);
        }
    }
}

FReply UDoodleOrganizeCompoundWidget::OneTouchDeleteRepectTexture()
{
    for (TSharedPtr<FTreeItem>  root: RootChildren)
    {
        if (root->Children.Num() > 1) 
        {
            TSharedPtr<FTreeItem> child = root->Children.Top();
            OnAssginRepeatTexture(child);
        }
    }
    GetAllRepetitiveTexture();
    return FReply::Handled();
}

void UDoodleOrganizeCompoundWidget::DeleteAllEmptyDirectory()
{
    TArray<FString> AssetPaths;
    IFileManager::Get().IterateDirectoryRecursively(*FPaths::ProjectContentDir(),[&](const TCHAR* FilenameOrDirectory, bool bIsDirectory) -> bool
    {
        if (bIsDirectory) 
        {
            FString DirectoryStr = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);
            //FString DirectoryStr = FilenameOrDirectory;
            TArray<FString> Files;
            FString FinalPath = DirectoryStr / TEXT("*");
            IFileManager::Get().FindFiles(Files, *FinalPath, true, true);
            if (Files.Num() <= 0)
            {
                if (!AssetPaths.Contains(DirectoryStr))
                {
                    AssetPaths.Add(DirectoryStr);
                }
            }
        }
        return true;
    });
    //------------------------------------------
    TArray<FString> RemovePath;
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    for (FString Path : AssetPaths)
    {
        TArray<FAssetData> AssetDatas;
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        AssetRegistryModule.Get().GetAssetsByPath(FName(Path), AssetDatas, true);
        if (AssetDatas.Num() <= 0)
        {
            UE_LOG(LogTemp, Log, TEXT("空文件夹:  %s"), *Path);
            bool res = IFileManager::Get().DeleteDirectory(*Path, false, true);
            if (res) 
            {
                RemovePath.Add(Path);
                //-----------------------
                FString Info = FString::Format(TEXT("删除问价夹{0}成功"), { Path });
                FNotificationInfo L_Info{ FText::FromString(Info) };
                L_Info.FadeInDuration = 2.0f;  // 
                L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
                FSlateNotificationManager::Get().AddNotification(L_Info);
            }
            EditorAssetSubsystem->SaveDirectory(Path);
        }
    }
    if (RemovePath.Num() > 0)
    {
        DeleteAllEmptyDirectory();
    }
}

FReply UDoodleOrganizeCompoundWidget::GenerateModeFolders()
{
    //FContentBrowserModule& ContentModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    //-------------
    if (ModeFolderName.TrimEnd().IsEmpty())
    {
        FText  DialogText = FText::FromString(TEXT("角色拼音名称不能为空。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    FString ContentPath = TEXT("/Game/");
    FString FolderPath = FPaths::Combine(ContentPath, TEXT("Character/") + ModeFolderName.TrimEnd());
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    if (!EditorAssetSubsystem->DoesDirectoryExist(FolderPath))
    {
        EditorAssetSubsystem->MakeDirectory(FolderPath);
    }
    FARFilter LFilter{};
    LFilter.bRecursivePaths = true;
    LFilter.bRecursiveClasses = true;
    LFilter.PackagePaths.Add(FName{ GamePath });
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AllAsset;
    AssetRegistryModule.Get().GetAssets(LFilter, AllAsset);
    TArray<UObject*> SelectedObjs;
    for (FAssetData SelectedData : AllAsset)
    {
        if (SelectedData.GetClass()->IsChildOf<UStaticMesh>()
            || SelectedData.GetClass()->IsChildOf<USkeletalMesh>()
            || SelectedData.GetClass()->IsChildOf<UPhysicsAsset>()
            || SelectedData.GetClass()->IsChildOf<UAnimationAsset>()
            || SelectedData.GetClass()->IsChildOf<USkeleton>())
        {
            MakeDirectoryByTypeMode(FolderPath, SelectedData, TEXT("Meshs"));
        }
        else if (SelectedData.GetClass()->IsChildOf<UTexture>())
        {
            MakeDirectoryByTypeMode(FolderPath, SelectedData, TEXT("Texture"));
        }
        else if (SelectedData.GetClass()->IsChildOf<UMaterialInstance>())
        {
            MakeDirectoryByTypeMode(FolderPath, SelectedData, TEXT("Material"));
        }
        else if (SelectedData.GetClass() == UMaterial::StaticClass())
        {
            MakeDirectoryByTypeMode(FolderPath, SelectedData, TEXT("Material/Mu01"));
        }
        else if (SelectedData.GetClass() == UMaterialParameterCollection::StaticClass()
            || SelectedData.GetClass() == UMaterialFunction::StaticClass()
            || SelectedData.GetClass()->IsChildOf<UBlueprint>())
        {
            MakeDirectoryByTypeMode(FolderPath, SelectedData, TEXT("Material/Mate01"));
        }
        else
        {
            MakeDirectoryByTypeMode(FolderPath, SelectedData, TEXT("Other"));
        }
        SelectedObjs.Add(SelectedData.GetAsset());
    }
    //--------------------------
    FixupAllReferencers();
    EditorAssetSubsystem->SaveLoadedAssets(SelectedObjs);
    //--------------------------
    return FReply::Handled();
}

void UDoodleOrganizeCompoundWidget::MakeDirectoryByTypeMode(FString FolderPath, FAssetData SelectedData, FString DirectoryName)
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    FString Path = FPaths::Combine(FolderPath, DirectoryName);
    if (!EditorAssetSubsystem->DoesDirectoryExist(Path))
    {
        EditorAssetSubsystem->MakeDirectory(Path);
    }
    FName AssetName = SelectedData.AssetName;
    if (!EditorAssetSubsystem->DoesAssetExist(FPaths::Combine(Path, AssetName.ToString())))
    {
        AssetViewUtils::MoveAssets({ SelectedData.GetAsset() }, Path, SelectedData.PackagePath.ToString());
    }
    else
    {
        while (EditorAssetSubsystem->DoesAssetExist(FPaths::Combine(Path, AssetName.ToString())))
        {
            int Counter = AssetName.GetNumber();
            AssetName.SetNumber(++Counter);
        }
        FString targetPath = FPaths::Combine(Path, AssetName.ToString());
        if (EditorAssetSubsystem->RenameAsset(SelectedData.PackageName.ToString(), targetPath))
        {
            FString Info = FString::Format(TEXT("文件重命名为:{0}"), { FPaths::Combine(SelectedData.PackagePath.ToString(), AssetName.ToString()) });
            FNotificationInfo L_Info{ FText::FromString(Info) };
            L_Info.FadeInDuration = 2.0f;  // 
            L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
            FSlateNotificationManager::Get().AddNotification(L_Info);
        }
    }
}

FReply UDoodleOrganizeCompoundWidget::RemoveSuffix()
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    FContentBrowserModule& ContentModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    TArray<FAssetData> SelectedAsset;
    ContentModule.Get().GetSelectedAssets(SelectedAsset);
    if (SelectedAsset.Num() <= 0)
    {
        FText  DialogText = FText::FromString(TEXT("请先在内容浏览器中，选择一个文件。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    //----------------------
    for (FAssetData SelectedData : SelectedAsset) 
    {
        FString StrName = SelectedData.AssetName.ToString();
        int32 SepIdx = INDEX_NONE;
        if (StrName.FindLastChar(TEXT('_'), SepIdx))
        {
            FString Str = StrName.LeftChop(StrName.Len()-SepIdx);
            FString targetPath = FPaths::Combine(SelectedData.PackagePath.ToString(), Str);
            if (EditorAssetSubsystem->RenameAsset(SelectedData.PackageName.ToString(), targetPath))
            {
                FString Info = FString::Format(TEXT("文件重命名为:{0}"), { FPaths::Combine(SelectedData.PackagePath.ToString(), Str) });
                FNotificationInfo L_Info{ FText::FromString(Info) };
                L_Info.FadeInDuration = 2.0f;  // 
                L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
                FSlateNotificationManager::Get().AddNotification(L_Info);
            }
            else 
            {
                FString Info = FString::Format(TEXT("重命名文件:{0}失败,该命名已存在"), { FPaths::Combine(SelectedData.PackagePath.ToString(), Str) });
                FNotificationInfo L_Info{ FText::FromString(Info) };
                L_Info.FadeInDuration = 2.0f;  // 
                L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Error"));
                FSlateNotificationManager::Get().AddNotification(L_Info);
            }
            EditorAssetSubsystem->SaveLoadedAsset(SelectedData.GetAsset());
        }
    }
    return FReply::Handled();
}

FReply UDoodleOrganizeCompoundWidget::AddSuffix()
{
    UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    FContentBrowserModule& ContentModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    TArray<FAssetData> SelectedAsset;
    ContentModule.Get().GetSelectedAssets(SelectedAsset);
    if (SelectedAsset.Num() <= 0)
    {
        FText  DialogText = FText::FromString(TEXT("请先在内容浏览器中，选择一个文件。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    if (TheSuffix.TrimEnd().IsEmpty())
    {
        FText  DialogText = FText::FromString(TEXT("请填先写后缀。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    //----------------------
    for (FAssetData SelectedData : SelectedAsset)
    {
        FString StrName = SelectedData.AssetName.ToString() + TEXT("_") + TheSuffix;
        FString targetPath = FPaths::Combine(SelectedData.PackagePath.ToString(), StrName);
        if (EditorAssetSubsystem->RenameAsset(SelectedData.PackageName.ToString(), targetPath))
        {
            FString Info = FString::Format(TEXT("文件重命名为:{0}"), { FPaths::Combine(SelectedData.PackagePath.ToString(), targetPath) });
            FNotificationInfo L_Info{ FText::FromString(Info) };
            L_Info.FadeInDuration = 2.0f;  // 
            L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Note"));
            FSlateNotificationManager::Get().AddNotification(L_Info);
            EditorAssetSubsystem->SaveLoadedAsset(SelectedData.GetAsset());
        }
    }
    return FReply::Handled();
}