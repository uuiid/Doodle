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
                        SAssignNew(nameText, STextBlock)
                            .Text(FText::FromString(myObject ? myObject->Mesh->GetFullName() : FString(TEXT("None"))))
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
                        SAssignNew(selectText, STextBlock)
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
                                SAssignNew(thisListView, SListView< TSharedPtr<FString> >)
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

                                                FString new_name = InText.ToString();
                                                FString old_name = *InItem;
                                                if (!myObject->allVaraint.Contains(new_name))
                                                {
                                                    if (myObject->allVaraint.Contains(old_name))
                                                    {
                                                        FDATA data = myObject->allVaraint[old_name];
                                                        auto copy = myObject->allVaraint;
                                                        myObject->allVaraint.Empty();
                                                        for (auto& e : copy)
                                                        {
                                                            if (e.Key.Equals(old_name))
                                                            {
                                                                myObject->allVaraint.Add(new_name, data);
                                                            }
                                                            else
                                                            {
                                                                myObject->allVaraint.Add(e.Key, e.Value);
                                                            }
                                                        }
                                                        copy.Empty();
                                                        if (nowVaraint.Equals(old_name))
                                                        {
                                                            nowVaraint = new_name;
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
                                                                nowVaraint = *InItem;
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
                                                if (myObject)
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
                                                .Text(FText::FromName(InItem->slot))
                                                ];
                                            ContentBox->AddSlot()
                                                .AutoHeight()
                                                .HAlign(HAlign_Left)
                                                .VAlign(VAlign_Center)
                                                [SNew(SObjectPropertyEntryBox)
                                                .ObjectPath_Lambda([InItem]()
                                                    {
                                                        if (InItem)
                                                            return InItem->material.GetPathName();
                                                        else
                                                            return FString(TEXT(""));
                                                    }
                                                )
                                                .AllowedClass(UMaterialInterface::StaticClass())
                                                        .OnObjectChanged_Lambda([&, InItem](const FAssetData& AssetData) {
                                                        if (myObject)
                                                        {
                                                            int index = InItem.Get()->index;
                                                            FDATA arr = myObject->allVaraint[nowVaraint];
                                                            UObject* obj = AssetData.GetAsset();
                                                            /*TSharedPtr<UMaterial> sm = MakeShareable<UMaterial>(obj);*/
                                                            TObjectPtr<UMaterial> ui = Cast<UMaterial>(obj);
                                                            arr.varaints[index] = FSkeletalMaterial(ui);
                                                            myObject->allVaraint[nowVaraint] = arr;
                                                            MaterialItems[index].Get()->material = arr.varaints[index].MaterialInterface;
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
                                                TObjectPtr<UMaterialInterface> mat = inSelectItem->material;
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
    TArray<FAssetData> selectedAss;
    ContentBrowserModle.Get().GetSelectedAssets(selectedAss);
    if (selectedAss.Num() <= 0) 
    {
        FText  DialogText = FText::FromString(TEXT("请先在内容浏览器中，选择一个骨骼网格体。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    FAssetData data = selectedAss[0];
    //----------------------------------------------------
    if (data.GetClass()->IsChildOf<USkeletalMesh>()) 
    {
        UObject* meshObj = data.GetAsset();
        USkeletalMesh* mesh = Cast<USkeletalMesh>(meshObj);
        UDoodleVariantAssetUserData* user_data = mesh->GetAssetUserData<UDoodleVariantAssetUserData>();
        if (user_data && user_data->variantObj) 
        {
            TArray<FString> OutKeys;
            myObject = user_data->variantObj;
            myObject->allVaraint.GetKeys(OutKeys);
            nowVaraint = OutKeys[0];
        }
        else
        {
            FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
            FString out_name = MakeUniqueObjectName(mesh, UDoodleVariantObject::StaticClass(), data.AssetName).ToString();
            FString PackageName = data.PackagePath.ToString()+"/" + out_name;
            const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
            UPackage* package = CreatePackage(*PackageName);
            package->MarkPackageDirty();
            UDoodleVariantFactory* MyFactory = NewObject<UDoodleVariantFactory>(UDoodleVariantFactory::StaticClass());
            FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
            UObject* object = AssetToolsModule.Get().CreateAsset(out_name, PackagePath, UDoodleVariantObject::StaticClass(), MyFactory);
            myObject = Cast<UDoodleVariantObject>(object);
            if (myObject) 
            {
                UDoodleVariantAssetUserData* UserData = NewObject<UDoodleVariantAssetUserData>(mesh, NAME_None, RF_NoFlags);
                FAssetData variant_date = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(myObject));
                UserData->variantObj = myObject;
                mesh->AddAssetUserData(UserData);
            }
            else
            {
                return FReply::Handled();
            }
            //FAssetData variant_date = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(myObject));
            TArray<FSkeletalMaterial> trangeMat = mesh->GetMaterials();
            for (int m = 0; m < trangeMat.Num(); m++) {
                trangeMat[m] = mesh->GetMaterials()[m];
            }
            myObject->Mesh = mesh;
            myObject->Path = data.PackagePath;
            FDATA f;
            f.varaints = trangeMat;
            nowVaraint = TEXT("default");
            myObject->allVaraint.Add(nowVaraint, f);
            //----------------------------
            AssetRegistryModule.Get().AssetCreated(myObject);
        }
        //-----------------------------
        TArray<UObject*> Objects;
        Objects.Add(myObject);
        ContentBrowserModle.Get().SyncBrowserToAssets(Objects);
        //-------------
        nameText->SetText(FText::FromString(myObject->Mesh->GetPathName()));
        Items.Empty();
        for (auto& e : myObject->allVaraint)
        {
            TSharedPtr<FString> f = MakeShared<FString>(e.Key);
            Items.Add(f);
        }
        thisListView->RequestListRefresh();
        SetVariantInfo(nowVaraint);
    }
	return FReply::Handled();
}

void DoodleVariantCompoundWidget::SetSetVariantData(UDoodleVariantObject* obj)
{
    myObject = obj;
    //------------------------
    nameText->SetText(FText::FromString(myObject->Mesh->GetPathName()));
    //----------------
    Items.Empty();
    for (auto& e : myObject->allVaraint)
    {
        TSharedPtr<FString> f = MakeShared<FString>(e.Key);
        Items.Add(f);
    }
    thisListView->RequestListRefresh();
    //------------------------
    TArray<FString> OutKeys;
    myObject->allVaraint.GetKeys(OutKeys);
    nowVaraint = OutKeys[0];
    SetVariantInfo(nowVaraint);
}

void DoodleVariantCompoundWidget::SetVariantInfo(FString name)
{
    if (myObject->allVaraint.Contains(name))
    {
        nowVaraint = name;
        //-------------
        MaterialItems.Empty();
        if (myObject->allVaraint.Num() > 0)
        {
            FDATA arr = myObject->allVaraint[name];
            for (int i = 0;i < arr.varaints.Num();i++)
            {
                FSkeletalMaterial mat = arr.varaints[i];
                TSharedPtr < FMaterialItemData> mItem = MakeShareable(new FMaterialItemData());
                mItem.Get()->material = mat.MaterialInterface;
                mItem.Get()->index = i;
                mItem.Get()->slot = mat.MaterialSlotName;
                MaterialItems.Add(mItem);
            }
        }
        MaterialListView->RequestListRefresh();
    }
}

FReply DoodleVariantCompoundWidget::OnVariantAdd()
{
    if (!myObject) 
    {
        FText DialogText = FText::FromString("");
        DialogText = FText::FromString(TEXT("请先载入变体。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    if (myObject->allVaraint.Num() > 0) 
    {
        TArray<FString> OutKeys;
        myObject->allVaraint.GetKeys(OutKeys);
        //FString key = OutKeys[0];
        FDATA nd = myObject->allVaraint[nowVaraint];
        FString name = TEXT("New");
        while (OutKeys.Contains(name))
        {
            FName TestName = FName(name);
            int Counter = TestName.GetNumber();
            TestName.SetNumber(++Counter);
            name = TestName.ToString();
        }
        myObject->allVaraint.Add(name, nd);
        //----------------
        Items.Empty();
        for (auto& e : myObject->allVaraint)
        {
            TSharedPtr<FString> f = MakeShared<FString>(e.Key);
            Items.Add(f);
        }
        thisListView->RequestListRefresh();
        SetVariantInfo(name);
    }
      
    return FReply::Handled();
}

FReply DoodleVariantCompoundWidget::OnVariantAttach()
{
    if (!myObject)
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
    selectText->SetText(FText::FromString(skin->GetActorNameOrLabel()));
    TArray<FSkeletalMaterial> list = myObject->allVaraint[nowVaraint].varaints;
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

