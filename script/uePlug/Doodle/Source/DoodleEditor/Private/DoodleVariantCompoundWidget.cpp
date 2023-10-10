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
#include "Widgets/Views/STableRow.h"

const FName DoodleVariantCompoundWidget::Name{ TEXT("VariantCompoundWidget") };

DECLARE_DELEGATE_OneParam(FAssetDataParamDelegate, FAssetData);
DECLARE_DELEGATE_OneParam(FTextParamDelegate, FText);

class SVariantItemTableRow :public STableRow<TSharedPtr<FString>>
{
public:
    using Super = STableRow<TSharedPtr<FString>>;

    SLATE_BEGIN_ARGS(SVariantItemTableRow)
        : _InItem(), _OnTextCommittedEvent(), _OnClickedEvent() {}
        SLATE_ARGUMENT(TSharedPtr<FString>, InItem)
        SLATE_EVENT(FTextParamDelegate, OnTextCommittedEvent)
        SLATE_EVENT(FSimpleDelegate, OnClickedEvent)
    SLATE_END_ARGS()
    //-----------------------
    TSharedPtr<FString> InItem;
    FTextParamDelegate OnTextCommittedEvent;
    FSimpleDelegate OnClickedEvent;

    void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& In_OwnerTableView)
    {
        InItem = InArgs._InItem;
        OnTextCommittedEvent = InArgs._OnTextCommittedEvent;
        OnClickedEvent = InArgs._OnClickedEvent;
        Super::FArguments L_Arg{};
        Super::Construct(L_Arg, In_OwnerTableView);
    }

    void ConstructChildren(ETableViewMode::Type InOwnerTableMode, const TAttribute<FMargin>& InPadding, const TSharedRef<SWidget>& InContent) override
    {
        this->Content = InContent;
        InnerContentSlot = nullptr;
        this->ChildSlot
            .Padding(InPadding)
            [
                SNew(SHorizontalBox)
                    +SHorizontalBox::Slot()
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                    [
                        SNew(SEditableText)
                            .MinDesiredWidth(400)
                            .IsEnabled(false)
                            .Text(FText::FromString(*InItem))
                            .OnTextCommitted_Lambda([this](const FText& InText, const ETextCommit::Type InTextAction) 
                            {
                                OnTextCommittedEvent.ExecuteIfBound(InText);
                            })
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
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
                                            .Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("GenericCommands.Paste")).GetSmallIcon())
                                    ]
                                    .ToolTipText(FText::FromString(TEXT("粘贴变体")))
                                    .OnClicked_Lambda([this] 
                                    {
                                    OnClickedEvent.ExecuteIfBound();;
                                    return FReply::Handled();})
                            ]
                    ]
            ];
        InnerContentSlot = &ChildSlot.AsSlot();
    }
};

class SMaterialItemTableRow :public STableRow<TSharedPtr<FMaterialItemData>>
{
public:
    using Super = STableRow<TSharedPtr<FMaterialItemData>>;

    SLATE_BEGIN_ARGS(SMaterialItemTableRow)
        : _InItem(),_OnObjectChanged(){}
    SLATE_ARGUMENT(TSharedPtr<FMaterialItemData>, InItem)
    SLATE_EVENT(FAssetDataParamDelegate, OnObjectChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& In_OwnerTableView)
    {
        InItem = InArgs._InItem;
        OnObjectChanged = InArgs._OnObjectChanged;
        Super::FArguments L_Arg{};
        Super::Construct(L_Arg, In_OwnerTableView);
    }

    void ConstructChildren(ETableViewMode::Type InOwnerTableMode, const TAttribute<FMargin>& InPadding, const TSharedRef<SWidget>& InContent) override
    {
        //-----------------------
        this->Content = InContent;
        InnerContentSlot = nullptr;
        this->ChildSlot
        .Padding(InPadding)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush(TEXT("NoBorder")))
            .Padding(0)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                    [SNew(STextBlock)
                    .Text(FText::FromName(InItem->Slot))
                    ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                    [
                        SNew(SObjectPropertyEntryBox)
                            .ObjectPath_Lambda([this]()
                                {
                                    if (InItem)
                                        return InItem->Material.GetPathName();
                                    else
                                        return FString(TEXT(""));
                                }
                            )
                            .AllowedClass(UMaterialInterface::StaticClass())
                            .OnObjectChanged_Lambda([this](const FAssetData& AssetData) {
                            if (AssetData.IsValid())
                            {
                                OnObjectChanged.ExecuteIfBound(AssetData);
                            }
                                })
                            .AllowClear(true)
                            .DisplayUseSelected(true)
                            .DisplayBrowse(true)
                            .ThumbnailPool(UThumbnailManager::Get().GetSharedThumbnailPool())
                    ]
            ]
        ];
        InnerContentSlot = &ChildSlot.AsSlot();
    }

    TSharedPtr<FMaterialItemData> InItem;
    FAssetDataParamDelegate OnObjectChanged;
};

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
                                    .OnGenerateRow(this, &DoodleVariantCompoundWidget::VariantListOnGenerateRow)
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
                                    .OnMouseButtonDoubleClick_Lambda([&](TSharedPtr<FString> inSelectItem)
                                    {
                                        TSharedPtr<ITableRow> TableRow = ThisListView->WidgetFromItem(inSelectItem);
                                        if (TableRow.IsValid())
                                        {
                                            TSharedPtr<SWidget> L_TableRow = TableRow->AsWidget();
                                            if (L_TableRow.IsValid())
                                            {
                                                TSharedPtr<SWidget> HorBox = L_TableRow->GetChildren()->GetChildAt(0);
                                                FChildren* HorWidgets = HorBox->GetChildren();
                                                for (int32 ChildItr = 0; ChildItr < HorWidgets->Num(); ChildItr++)
                                                {
                                                    TSharedPtr<SWidget> Child = HorWidgets->GetChildAt(ChildItr);
                                                    if (Child->GetWidgetClass().GetWidgetType().IsEqual(SEditableText::StaticWidgetClass().GetWidgetType()))
                                                    {
                                                        TSharedPtr<SEditableText> text = StaticCastSharedPtr<SEditableText>(Child);
                                                        text->SetEnabled(true);
                                                    }
                                                }
                                            }
                                        }
                                    })
                                    .OnContextMenuOpening_Lambda([this]() {
                                        FUIAction ActionDelete(FExecuteAction::CreateRaw(this, &DoodleVariantCompoundWidget::OnVariantDelete), FCanExecuteAction());
                                        FMenuBuilder MenuBuilder(true, false);
                                        MenuBuilder.AddMenuSeparator();
                                        MenuBuilder.AddMenuEntry(FText::FromString(TEXT("删除")), FText::FromString(TEXT("删除变体")), 
                                            FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Delete")), ActionDelete);
                                        return MenuBuilder.MakeWidget();
                                     })
                                    .SelectionMode(ESelectionMode::Type::Single)
                                    .HeaderRow
                                    (
                                        SNew(SHeaderRow)
                                        + SHeaderRow::Column(TEXT("Name"))
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
                                        + SHeaderRow::Column(TEXT("Tool"))
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
                                    .OnGenerateRow(this, &DoodleVariantCompoundWidget::MaterialListOnGenerateRow)
                                    .OnSelectionChanged_Lambda([](TSharedPtr<FMaterialItemData> inSelectItem, ESelectInfo::Type SelectType)
                                    {
                                        if (inSelectItem)
                                        {
                                            TObjectPtr<UMaterialInterface> mat = inSelectItem->Material;
                                            ESelectInfo::Type t = SelectType;
                                        }
                                    })
                                    .SelectionMode(ESelectionMode::Type::Single)
                                    .HeaderRow
                                    (
                                        SNew(SHeaderRow)
                                        + SHeaderRow::Column(TEXT("Name"))
                                        [
                                            SNew(SBorder)
                                                .Padding(5)
                                                .Content()
                                                [
                                                    SNew(STextBlock)
                                                        .Text(FText::FromString(TEXT("插槽")))
                                                ]
                                        ]
                                        + SHeaderRow::Column(TEXT("Number")).DefaultLabel(FText::FromString(TEXT("材质")))
                                    )
                            ]
                    ]
            ]
       
	];
    
}

TSharedRef<ITableRow> DoodleVariantCompoundWidget::VariantListOnGenerateRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(SVariantItemTableRow, OwnerTable)
        .InItem(InItem)
        .OnTextCommittedEvent_Lambda([this,InItem](FText InText)
        {
            VariantNameOnTextCommitted(InText, InItem);
        })
        .OnClickedEvent_Lambda([this,InItem]() 
        {
            NowVaraint = *InItem;
            OnVariantAttach();
        });
}

void DoodleVariantCompoundWidget::VariantNameOnTextCommitted(const FText& InText, TSharedPtr<FString> InItem)
{
    FString NewName = InText.ToString();
    FString OldName = *InItem;
    TSharedPtr<SEditableText> text = nullptr;
    //-----------------------
    TSharedPtr<ITableRow> TableRow = ThisListView->WidgetFromItem(InItem);
    if (TableRow.IsValid())
    {
        TSharedPtr<SWidget> L_TableRow = TableRow->AsWidget();
        if (L_TableRow.IsValid())
        {
            TSharedPtr<SWidget> HorBox = L_TableRow->GetChildren()->GetChildAt(0);
            FChildren* HorWidgets = HorBox->GetChildren();
            for (int32 ChildItr = 0; ChildItr < HorWidgets->Num(); ChildItr++)
            {
                TSharedPtr<SWidget> Child = HorWidgets->GetChildAt(ChildItr);
                if (Child->GetWidgetClass().GetWidgetType().IsEqual(SEditableText::StaticWidgetClass().GetWidgetType()))
                {
                    text = StaticCastSharedPtr<SEditableText>(Child);
                    text->SetEnabled(false);
                }
            }
        }
    }
    //----------------------------------
    if (!CurrentObject->AllVaraint.Contains(NewName))
    {
        if (CurrentObject->AllVaraint.Contains(OldName))
        {
            FVariantInfo L_Data = CurrentObject->AllVaraint[OldName];
            CurrentObject->AllVaraint.Remove(OldName);    // Get rid of the old key (and it's value)
            CurrentObject->AllVaraint.Shrink();
            CurrentObject->AllVaraint.Add(NewName, L_Data);
            if (NowVaraint.Equals(OldName))
            {
                NowVaraint = NewName;
            }
        }
    }
    Items.Empty();
    for (auto& e : CurrentObject->AllVaraint)
    {
        TSharedPtr<FString> f = MakeShared<FString>(e.Key);
        Items.Add(f);
    }
    ThisListView->RequestListRefresh();
}

TSharedRef<ITableRow> DoodleVariantCompoundWidget::MaterialListOnGenerateRow(TSharedPtr<FMaterialItemData> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(SMaterialItemTableRow, OwnerTable)
        .InItem(InItem)
        .OnObjectChanged_Lambda([this, InItem](FAssetData AssetData)
        {
            if (CurrentObject)
            {
                int index = InItem->Index;
                FVariantInfo Arr = CurrentObject->AllVaraint[NowVaraint];
                TObjectPtr<UMaterial> ui = Cast<UMaterial>(AssetData.GetAsset());
                Arr.Variants[index] = FSkeletalMaterial(ui);
                CurrentObject->AllVaraint[NowVaraint] = Arr;
                MaterialItems[index]->Material = Arr.Variants[index].MaterialInterface;
            }
        });
}
//-------------------------------------------
FReply DoodleVariantCompoundWidget::OnLoadAllVariant()
{
    FContentBrowserModule& ContentBrowserModle = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
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
            FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
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
            TSharedPtr<FString> Str = MakeShared<FString>(e.Key);
            Items.Add(Str);
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
                TSharedPtr < FMaterialItemData> MatItem = MakeShareable(new FMaterialItemData());
                MatItem->Material = Mat.MaterialInterface;
                MatItem->Index = i;
                MatItem->Slot = Mat.MaterialSlotName;
                MaterialItems.Add(MatItem);
            }
        }
        MaterialListView->RequestListRefresh();
    }
}

FReply DoodleVariantCompoundWidget::OnVariantAdd()
{
    if (!CurrentObject)
    {
        FText DialogText = {};
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
        FName L_Name = TEXT("New");
        while (OutKeys.Contains(L_Name.ToString()))
        {
            int Counter = L_Name.GetNumber();
            L_Name.SetNumber(++Counter);
        }
        CurrentObject->AllVaraint.Add(L_Name.ToString(), Nd);
        //----------------
        Items.Empty();
        for (auto& e : CurrentObject->AllVaraint)
        {
            TSharedPtr<FString> Str = MakeShared<FString>(e.Key);
            Items.Add(Str);
        }
        ThisListView->RequestListRefresh();
        SetVariantInfo(L_Name.ToString());
    }
      
    return FReply::Handled();
}

void DoodleVariantCompoundWidget::OnVariantDelete()
{
    const TArray<TSharedPtr<FString>> Selection = ThisListView->GetSelectedItems();
    if (!Selection.IsEmpty())
    {
        if (CurrentObject->AllVaraint.Num() > 0)
        {
            for (TSharedPtr<FString> L_Key : Selection)
            {
                if (CurrentObject->AllVaraint.Contains(*L_Key))
                {
                    CurrentObject->AllVaraint.Remove(*L_Key);
                    //--------------
                    Items.Empty();
                    for (auto& e : CurrentObject->AllVaraint)
                    {
                        TSharedPtr<FString> Str = MakeShared<FString>(e.Key);
                        Items.Add(Str);
                    }
                    ThisListView->RequestListRefresh();
                    //------------------------
                    TArray<FString> OutKeys;
                    CurrentObject->AllVaraint.GetKeys(OutKeys);
                    NowVaraint = OutKeys[0];
                    SetVariantInfo(NowVaraint);
                }
            }
            Items.Empty();
            for (auto& Ve : CurrentObject->AllVaraint)
            {
                TSharedPtr<FString> Str = MakeShared<FString>(Ve.Key);
                Items.Add(Str);
            }
            ThisListView->RequestListRefresh();
            //------------------------
            TArray<FString> OutKeys;
            CurrentObject->AllVaraint.GetKeys(OutKeys);
            NowVaraint = OutKeys[0];
            SetVariantInfo(NowVaraint);
        }
    }
}

FReply DoodleVariantCompoundWidget::OnVariantAttach()
{
    if (!CurrentObject)
    {
        FText DialogText = FText::FromString(TEXT(""));
        DialogText = FText::FromString(TEXT("请先载入变体。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    //----------------------
    USelection* L_Selects = GEditor->GetSelectedActors();
    TArray<UObject*> outObject;
    L_Selects->GetSelectedObjects(outObject);
    if (outObject.Num() <= 0 || !outObject[0]->GetClass()->IsChildOf<ASkeletalMeshActor>())
    {
        FText DialogText = FText::FromString(TEXT(""));
        DialogText = FText::FromString(TEXT("请先选择粘贴目标。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }

    UObject* L_Target = L_Selects->GetSelectedObject(0);
    ASkeletalMeshActor* L_Skin = Cast<ASkeletalMeshActor>(L_Target);
    SelectText->SetText(FText::FromString(L_Skin->GetActorNameOrLabel()));
    TArray<FSkeletalMaterial> L_List = CurrentObject->AllVaraint[NowVaraint].Variants;
    for (int i = 0;i < L_List.Num();i++)
    {
        L_Skin->GetSkeletalMeshComponent()->SetMaterial(i, L_List[i].MaterialInterface);
    }
    L_Skin->GetSkeletalMeshComponent()->PostApplyToComponent();
   //-----------------------
    return FReply::Handled();
}

TSharedRef<SDockTab> DoodleVariantCompoundWidget::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(DoodleVariantCompoundWidget)];  // 这里创建我们自己的界面
}

