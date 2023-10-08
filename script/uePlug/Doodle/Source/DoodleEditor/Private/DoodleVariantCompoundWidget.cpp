// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleVariantCompoundWidget.h"
#include "SlateOptMacros.h"

#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "GeometryCache.h"

#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "Factories/BlueprintFactory.h"
#include "DoodleVariantFactory.h"
#include "AssetTypeActions_Base.h"

#include "LevelEditor.h"
#include "Animation/SkeletalMeshActor.h"
#include "SequencerUtilities.h"
#include "IMovieSceneTools.h"
#include "MovieSceneToolsModule.h"
#include "Selection.h"
#include "DoodleVariantAssetUserData.h"
#include "Textures/SlateIcon.h"

const FName DoodleVariantCompoundWidget::Name{ TEXT("VariantCompoundWidget") };


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void DoodleVariantCompoundWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
        SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                            .Text(FText::FromString(TEXT("骨骼网格体:")))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .VAlign(VAlign_Top)
                    [
                        SAssignNew(NameText, STextBlock)
                            .Text(FText::FromString(CurrentObject ? CurrentObject->Mesh->GetFullName() : FString(TEXT("None"))))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                            .Text(FText::FromString(TEXT("粘贴到目标:")))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .VAlign(VAlign_Top)
                    [
                        SAssignNew(SelectText, STextBlock)
                            .Text(FText::FromString(FString(TEXT("None"))))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .VAlign(VAlign_Top)
                    [
                        SNew(SButton)
                            .Text(FText::FromString(TEXT("载入所有变体")))
                            .OnClicked(this, &DoodleVariantCompoundWidget::OnLoadAllVariant)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .VAlign(VAlign_Top)
                    [
                        SNew(STextBlock)
                            .Text(FText::FromString(FString(TEXT(""))))
                    ]
            ]
            + SVerticalBox::Slot()
            [
                SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    [
                        SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .VAlign(VAlign_Top)
                            [
                                SAssignNew(ThisListView, SListView< TSharedPtr<FString> >)
                                    .ItemHeight(24)
                                    .ListItemsSource(&Items)
                                    .OnGenerateRow_Lambda([this](TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable)->TSharedRef<ITableRow>
                                        {
                                            TSharedRef<STableRow<TSharedPtr<FString>>> TableRowWidget = SNew(STableRow<TSharedPtr<FString>>, OwnerTable);
                                            TSharedPtr<SHorizontalBox> ContentBox{};

                                            int index = OwnerTable.Get().GetNumGeneratedChildren();

                                            TSharedRef<SWidget> Content =
                                                SNew(SBorder)
                                                .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                                                .Padding(0)
                                                .Cursor(EMouseCursor::GrabHand)
                                                [SAssignNew(ContentBox, SHorizontalBox)];

                                            ContentBox->AddSlot()
                                                .HAlign(HAlign_Left)
                                                .VAlign(VAlign_Center)
                                                [SNew(SEditableTextBox)
                                                .Text(FText::FromString(*InItem))
                                                .OnTextCommitted_Lambda([&, InItem, this](const FText& InText, const ETextCommit::Type InTextAction) {

                                                FString NewName = InText.ToString();
                                                FString OldName = *InItem;
                                                if (!CurrentObject->AllVaraint.Contains(NewName))
                                                {
                                                    if (CurrentObject->AllVaraint.Contains(OldName))
                                                    {
                                                        FVariantInfo L_Data = CurrentObject->AllVaraint[OldName];
                                                        auto copy = CurrentObject->AllVaraint;
                                                        CurrentObject->AllVaraint.Empty();
                                                        for (auto& e : copy)
                                                        {
                                                            if (e.Key.Equals(OldName))
                                                            {
                                                                CurrentObject->AllVaraint.Add(NewName, L_Data);
                                                            }
                                                            else
                                                            {
                                                                CurrentObject->AllVaraint.Add(e.Key, e.Value);
                                                            }
                                                        }
                                                        copy.Empty();
                                                        if (NowVaraint.Equals(OldName))
                                                        {
                                                            NowVaraint = NewName;
                                                        }
                                                    }
                                                }
                                                    })
                                                ];
                                            ContentBox->AddSlot()
                                                .HAlign(HAlign_Right)
                                                .VAlign(VAlign_Center)
                                                [
                                                    SNew(SHorizontalBox)
                                                        + SHorizontalBox::Slot()
                                                        .AutoWidth()
                                                        [
                                                            SNew(SButton)
                                                                .Text(FText::FromString(TEXT("粘贴")))
                                                                .Content()
                                                                [
                                                                    SNew(SImage)
                                                                        .Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Paste").GetSmallIcon())
                                                                ]
                                                                .ToolTipText(FText::FromString(TEXT("粘贴变体")))
                                                                .OnClicked_Lambda([this, InItem] {
                                                                NowVaraint = *InItem;
                                                                OnVariantAttach();
                                                                return FReply::Handled();})
                                                        ]
                                                ];
                                            TableRowWidget->SetContent(Content);
                                            return TableRowWidget;
                                        })
                                    .OnSelectionChanged_Lambda([&](TSharedPtr<FString> inSelectItem, ESelectInfo::Type SelectType)
                                        {
                                            if (inSelectItem)
                                            {
                                                FString name = *inSelectItem;
                                                if (CurrentObject)
                                                {
                                                    SetVariantInfo(name);
                                                }
                                                ESelectInfo::Type t = SelectType;
                                            }
                                        })
                                            .SelectionMode(ESelectionMode::Type::Single)
                                            .HeaderRow
                                            (
                                                SNew(SHeaderRow)
                                                + SHeaderRow::Column("Name")
                                                .HAlignHeader(HAlign_Left)
                                                .HAlignCell(HAlign_Left)
                                                [
                                                    SNew(SHorizontalBox)
                                                        + SHorizontalBox::Slot()
                                                        .HAlign(HAlign_Left)
                                                        .VAlign(VAlign_Center)
                                                        .AutoWidth()
                                                        [
                                                            SNew(STextBlock)
                                                                .Text(FText::FromString(TEXT("变体列表:")))
                                                        ]
                                                ]
                                                + SHeaderRow::Column("Tool")
                                                .HAlignHeader(HAlign_Right)
                                                .HAlignCell(HAlign_Right)
                                                [
                                                    SNew(SHorizontalBox)
                                                        + SHorizontalBox::Slot()
                                                        .HAlign(HAlign_Right)
                                                        .AutoWidth()
                                                        [
                                                            SNew(SButton)
                                                                .Content()
                                                                [
                                                                    SNew(SImage)
                                                                        .Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "WorldBrowser.NewFolderIcon").GetSmallIcon())
                                                                ]
                                                                .Text(FText::FromString(TEXT("添加")))
                                                                .ToolTipText(FText::FromString(TEXT("添加变体")))
                                                                .OnClicked(this, &DoodleVariantCompoundWidget::OnVariantAdd)
                                                        ]
                                                ]
                                            )
                            ]

                    ]
                    + SHorizontalBox::Slot()
                    .HAlign(HAlign_Fill)
                    [
                        SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .VAlign(VAlign_Fill)
                            [
                                SAssignNew(MaterialListView, SListView<TSharedPtr< FMaterialItemData> >)
                                    .ListItemsSource(&MaterialItems)
                                    .OnGenerateRow_Lambda([&](TSharedPtr<FMaterialItemData> InItem, const TSharedRef<STableViewBase>& OwnerTable)->TSharedRef<ITableRow>
                                        {
                                            TSharedRef<STableRow<TSharedPtr<FMaterialItemData>>> TableRowWidget = SNew(STableRow<TSharedPtr<FMaterialItemData>>, OwnerTable);
                                            TSharedPtr<SVerticalBox> ContentBox{};
                                            TSharedRef<SWidget> Content =
                                                SNew(SBorder)
                                                .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                                                .Padding(0)
                                                .Cursor(EMouseCursor::GrabHand)
                                                [SAssignNew(ContentBox, SVerticalBox)];
                                            ContentBox->AddSlot()
                                                .AutoHeight()
                                                .HAlign(HAlign_Left)
                                                .VAlign(VAlign_Center)
                                                [SNew(STextBlock)
                                                .Text(FText::FromName(InItem->Slot))
                                                ];
                                            ContentBox->AddSlot()
                                                .AutoHeight()
                                                .HAlign(HAlign_Left)
                                                .VAlign(VAlign_Center)
                                                [SNew(SObjectPropertyEntryBox)
                                                .ObjectPath_Lambda([InItem]()
                                                    {
                                                        if (InItem)
                                                            return InItem->Material.GetPathName();
                                                        else
                                                            return FString(TEXT(""));
                                                    }
                                                )
                                                .AllowedClass(UMaterialInterface::StaticClass())
                                                        .OnObjectChanged_Lambda([&, InItem](const FAssetData& AssetData) {
                                                        if (CurrentObject)
                                                        {
                                                            int index = InItem.Get()->Index;
                                                            FVariantInfo Arr = CurrentObject->AllVaraint[NowVaraint];
                                                            UObject* obj = AssetData.GetAsset();
                                                            TObjectPtr<UMaterial> ui = Cast<UMaterial>(obj);
                                                            Arr.Variants[index] = FSkeletalMaterial(ui);
                                                            CurrentObject->AllVaraint[NowVaraint] = Arr;
                                                            MaterialItems[index].Get()->Material = Arr.Variants[index].MaterialInterface;
                                                        }
                                                            })
                                                        .AllowClear(true)
                                                                .DisplayUseSelected(true)
                                                                .DisplayBrowse(true)
                                                                .ThumbnailPool(UThumbnailManager::Get().GetSharedThumbnailPool())
                                                                //.IsEnabled(this, &SMaterialAnalyzer::IsMaterialSelectionAllowed);
                                                ];
                                            TableRowWidget->SetContent(Content);
                                            return TableRowWidget;
                                        }
                                    )
                                    .OnSelectionChanged_Lambda([](TSharedPtr<FMaterialItemData> inSelectItem, ESelectInfo::Type SelectType)
                                        {
                                            if (inSelectItem)
                                            {
                                                TObjectPtr<UMaterialInterface> mat = inSelectItem->Material;
                                                ESelectInfo::Type t = SelectType;
                                            }
                                            //UE_LOG("LogSizeof", Log, TEXT("Select Item, type = %s, size = %s"), *inSelectItem, TEXT("111"))
                                        })
                                            .SelectionMode(ESelectionMode::Type::Single)
                                            .HeaderRow
                                            (
                                                SNew(SHeaderRow)
                                                + SHeaderRow::Column("Name")
                                                [
                                                    SNew(SBorder)
                                                        .Padding(5)
                                                        .Content()
                                                        [
                                                            SNew(STextBlock)
                                                                .Text(FText::FromString(TEXT("插槽")))
                                                        ]
                                                ]
                                                + SHeaderRow::Column("Number").DefaultLabel(FText::FromString(TEXT("材质")))
                                            )
                            ]
                    ]
            ]
       
	];
    
}

FReply DoodleVariantCompoundWidget::OnLoadAllVariant()
{
    FContentBrowserModule& ContentBrowserModle = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>( "ContentBrowser");
    TArray<FAssetData> SelectedAsset;
    ContentBrowserModle.Get().GetSelectedAssets(SelectedAsset);
    if (SelectedAsset.Num() <= 0)
    {
        FText  DialogText = FText::FromString(TEXT("请先在内容浏览器中，选择一个骨骼网格体。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    FAssetData SelectedData = SelectedAsset[0];
    //----------------------------------------------------
    if (SelectedData.GetClass()->IsChildOf<USkeletalMesh>())
    {
        UObject* MeshObj = SelectedData.GetAsset();
        USkeletalMesh* L_Mesh = Cast<USkeletalMesh>(MeshObj);
        UDoodleVariantAssetUserData* UserData = L_Mesh->GetAssetUserData<UDoodleVariantAssetUserData>();
        if (UserData && UserData->VariantObj)
        {
            TArray<FString> OutKeys;
            CurrentObject = UserData->VariantObj;
            CurrentObject->AllVaraint.GetKeys(OutKeys);
            NowVaraint = OutKeys[0];
        }
        else
        {
            FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
            FString out_name = MakeUniqueObjectName(L_Mesh, UDoodleVariantObject::StaticClass(), SelectedData.AssetName).ToString();
            FString PackageName = SelectedData.PackagePath.ToString()+"/" + out_name;
            const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
            UPackage* package = CreatePackage(*PackageName);
            package->MarkPackageDirty();
            UDoodleVariantFactory* MyFactory = NewObject<UDoodleVariantFactory>(UDoodleVariantFactory::StaticClass());
            FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
            UObject* object = AssetToolsModule.Get().CreateAsset(out_name, PackagePath, UDoodleVariantObject::StaticClass(), MyFactory);
            CurrentObject = Cast<UDoodleVariantObject>(object);
            if (CurrentObject)
            {
                UserData = NewObject<UDoodleVariantAssetUserData>(L_Mesh, NAME_None, RF_NoFlags);
                FAssetData variant_date = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(CurrentObject));
                UserData->VariantObj = CurrentObject;
                L_Mesh->AddAssetUserData(UserData);
            }
            else
            {
                return FReply::Handled();
            }
            TArray<FSkeletalMaterial> trangeMat = L_Mesh->GetMaterials();
            for (int M = 0; M < trangeMat.Num(); M++) {
                trangeMat[M] = L_Mesh->GetMaterials()[M];
            }
            CurrentObject->Mesh = L_Mesh;
            CurrentObject->Path = SelectedData.PackagePath;
            FVariantInfo Info;
            Info.Variants = trangeMat;
            NowVaraint = TEXT("default");
            CurrentObject->AllVaraint.Add(NowVaraint, Info);
            //----------------------------
            AssetRegistryModule.Get().AssetCreated(CurrentObject);
        }
        //-----------------------------
        TArray<UObject*> Objects;
        Objects.Add(CurrentObject);
        ContentBrowserModle.Get().SyncBrowserToAssets(Objects);
        //-------------
        NameText->SetText(FText::FromString(CurrentObject->Mesh->GetPathName()));
        Items.Empty();
        for (auto& e : CurrentObject->AllVaraint)
        {
            TSharedPtr<FString> f = MakeShared<FString>(e.Key);
            Items.Add(f);
        }
        ThisListView->RequestListRefresh();
        SetVariantInfo(NowVaraint);
    }
	return FReply::Handled();
}

void DoodleVariantCompoundWidget::SetSetVariantData(UDoodleVariantObject* obj)
{
    CurrentObject = obj;
    //------------------------
    NameText->SetText(FText::FromString(CurrentObject->Mesh->GetPathName()));
    //----------------
    Items.Empty();
    for (auto& e : CurrentObject->AllVaraint)
    {
        TSharedPtr<FString> f = MakeShared<FString>(e.Key);
        Items.Add(f);
    }
    ThisListView->RequestListRefresh();
    //------------------------
    TArray<FString> OutKeys;
    CurrentObject->AllVaraint.GetKeys(OutKeys);
    NowVaraint = OutKeys[0];
    SetVariantInfo(NowVaraint);
}

void DoodleVariantCompoundWidget::SetVariantInfo(FString name)
{
    if (CurrentObject->AllVaraint.Contains(name))
    {
        NowVaraint = name;
        //-------------
        MaterialItems.Empty();
        if (CurrentObject->AllVaraint.Num() > 0)
        {
            FVariantInfo Arr = CurrentObject->AllVaraint[name];
            for (int i = 0;i < Arr.Variants.Num();i++)
            {
                FSkeletalMaterial Mat = Arr.Variants[i];
                TSharedPtr < FMaterialItemData> mItem = MakeShareable(new FMaterialItemData());
                mItem.Get()->Material = Mat.MaterialInterface;
                mItem.Get()->Index = i;
                mItem.Get()->Slot = Mat.MaterialSlotName;
                MaterialItems.Add(mItem);
            }
        }
        MaterialListView->RequestListRefresh();
    }
}

FReply DoodleVariantCompoundWidget::OnVariantAdd()
{
    if (!CurrentObject)
    {
        FText DialogText = FText::FromString("");
        DialogText = FText::FromString(TEXT("请先载入变体。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    if (CurrentObject->AllVaraint.Num() > 0)
    {
        TArray<FString> OutKeys;
        CurrentObject->AllVaraint.GetKeys(OutKeys);
        //FString key = OutKeys[0];
        FVariantInfo Nd = CurrentObject->AllVaraint[NowVaraint];
        FString L_Name = TEXT("New");
        while (OutKeys.Contains(L_Name))
        {
            FName TestName = FName(L_Name);
            int Counter = TestName.GetNumber();
            TestName.SetNumber(++Counter);
            L_Name = TestName.ToString();
        }
        CurrentObject->AllVaraint.Add(L_Name, Nd);
        //----------------
        Items.Empty();
        for (auto& e : CurrentObject->AllVaraint)
        {
            TSharedPtr<FString> Str = MakeShared<FString>(e.Key);
            Items.Add(Str);
        }
        ThisListView->RequestListRefresh();
        SetVariantInfo(L_Name);
    }
      
    return FReply::Handled();
}

FReply DoodleVariantCompoundWidget::OnVariantAttach()
{
    if (!CurrentObject)
    {
        FText DialogText = FText::FromString("");
        DialogText = FText::FromString(TEXT("请先载入变体。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    //----------------------
    USelection* selects = GEditor->GetSelectedActors();
    TArray<UObject*> outObject;
    selects->GetSelectedObjects(outObject);
    if (outObject.Num() <= 0 || !outObject[0]->GetClass()->IsChildOf<ASkeletalMeshActor>())
    {
        FText DialogText = FText::FromString("");
        DialogText = FText::FromString(TEXT("请先选择粘贴目标。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }

    UObject* target = selects->GetSelectedObject(0);
    ASkeletalMeshActor* skin = Cast<ASkeletalMeshActor>(target);
    SelectText->SetText(FText::FromString(skin->GetActorNameOrLabel()));
    TArray<FSkeletalMaterial> list = CurrentObject->AllVaraint[NowVaraint].Variants;
    for (int i = 0;i < list.Num();i++)
    {
        skin->GetSkeletalMeshComponent()->SetMaterial(i, list[i].MaterialInterface);
    }
    skin->GetSkeletalMeshComponent()->PostApplyToComponent();
    //    // Get the level editor module
   //-----------------------
    return FReply::Handled();
}

TSharedRef<SDockTab> DoodleVariantCompoundWidget::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(DoodleVariantCompoundWidget)];  // 这里创建我们自己的界面
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

