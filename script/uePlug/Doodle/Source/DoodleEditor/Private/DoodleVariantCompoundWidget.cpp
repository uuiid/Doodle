// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleVariantCompoundWidget.h"

#include "Animation/SkeletalMeshActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_Base.h"
#include "DoodleVariantAssetUserData.h"
#include "DoodleVariantEditorViewport.h"
#include "DoodleVariantFactory.h"
#include "ContentBrowserModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "Factories/BlueprintFactory.h"
#include "GeometryCache.h"
#include "IMovieSceneTools.h"
#include "LevelEditor.h"
#include "MovieSceneToolsModule.h"
#include "Selection.h"
#include "SequencerUtilities.h"
#include "SlateOptMacros.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Views/STableRow.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Engine/SkinnedAssetCommon.h"

const FName DoodleVariantCompoundWidget::Name{ TEXT("VariantCompoundWidget") };

DECLARE_DELEGATE_TwoParams(FAssetDataParamDelegate, FAssetData,FName);
DECLARE_DELEGATE_OneParam(FTextParamDelegate, FText);
DECLARE_DELEGATE_OneParam(FECheckBoxStateParamDelegate, ECheckBoxState);
DECLARE_DELEGATE_RetVal(ECheckBoxState,FReturnECheckBoxStateParam);

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
      // clang-format off
        InItem = InArgs._InItem;
        OnTextCommittedEvent = InArgs._OnTextCommittedEvent;
        OnClickedEvent = InArgs._OnClickedEvent;
        //---------------------------------------
        Super::FArguments L_Arg{};
        L_Arg.Content()[
            SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
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
                        OnClickedEvent.ExecuteIfBound();
                        return FReply::Handled();
                    })
                ]
        ];
        Super::Construct(L_Arg, In_OwnerTableView);
    }
};

class SMaterialItemTableRow :public STableRow<TSharedPtr<FMaterialItemData>>
{
public:
    using Super = STableRow<TSharedPtr<FMaterialItemData>>;

    SLATE_BEGIN_ARGS(SMaterialItemTableRow)
        : _InItem(),_OnObjectChanged(),_OnCheckStateChanged(),_GetCheckState(){}
    SLATE_ARGUMENT(TSharedPtr<FMaterialItemData>, InItem)
    SLATE_EVENT(FAssetDataParamDelegate, OnObjectChanged)
    SLATE_EVENT(FECheckBoxStateParamDelegate, OnCheckStateChanged)
    SLATE_EVENT(FReturnECheckBoxStateParam, GetCheckState)
    SLATE_END_ARGS()
    //--------------------------
    TSharedPtr<FMaterialItemData> InItem;
    FAssetDataParamDelegate OnObjectChanged;
    FECheckBoxStateParamDelegate OnCheckStateChanged;
    FReturnECheckBoxStateParam GetCheckState;

    void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& In_OwnerTableView)
    {
      // clang-format off
        InItem = InArgs._InItem;
        OnObjectChanged = InArgs._OnObjectChanged;
        OnCheckStateChanged = InArgs._OnCheckStateChanged;
        GetCheckState = InArgs._GetCheckState;
        //------------------
        Super::FArguments L_Arg{};
        L_Arg.Content()
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush(TEXT("NoBorder")))
            .Padding(2,0,0,0)
            [
              SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                .FillWidth(0.3f)
                [
                    SNew(SCheckBox)
                        .IsChecked_Lambda([this]() 
                        {
                            if (GetCheckState.IsBound())
                                return GetCheckState.Execute();
                            else
                                return ECheckBoxState::Undetermined;
                        })
                        .OnCheckStateChanged_Lambda([this](ECheckBoxState state)
                        {
                            OnCheckStateChanged.ExecuteIfBound(state);
                        })
                        .ToolTipText(FText::FromString(TEXT("Isolates this material in the viewport")))
                        [
                            SNew(STextBlock)
                                .ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f))
                                .Text(FText::FromString(TEXT("独显")))
                        ]
                ]
                +SHorizontalBox::Slot()
                [
                    SNew(SObjectPropertyEntryBox)
                        .ObjectPath_Lambda([this]() 
                        {
                            if (InItem)
                                return InItem->Material.GetPathName();
                            else
                                return FString(TEXT(""));
                        })
                        .AllowedClass(UMaterialInterface::StaticClass())
                        .OnObjectChanged_Lambda([this](const FAssetData& AssetData) 
                        {
                            if (AssetData.IsValid()) 
                            {
                                OnObjectChanged.ExecuteIfBound(AssetData, InItem->Slot);
                            }
                        })
                        .AllowClear(true)
                        .DisplayUseSelected(true)
                        .DisplayBrowse(true)
                        .ThumbnailPool(UThumbnailManager::Get().GetSharedThumbnailPool())
                        .CustomContentSlot()
                        [
                            SNew(STextBlock)
                                .Text(FText::FromName(InItem->Slot))
                        ]
                ]
            ]
        ];
        Super::Construct(L_Arg, In_OwnerTableView);
    }
};

void DoodleVariantCompoundWidget::Construct(const FArguments& InArgs)
{
  // clang-format off
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
                            .Text(FText::FromString(TEXT("对应骨骼网格体")))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .VAlign(VAlign_Top)
                    [
                        SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            [
                                SAssignNew(NameText, STextBlock)
                                    .Text(FText::FromString(CurrentObject ? CurrentObject->Mesh->GetFullName() : FString(TEXT("None"))))
                            ]
                            + SHorizontalBox::Slot()
                            [
                                SAssignNew(ButtonLinkMesh, SButton)
                                    .Visibility(EVisibility::Hidden)
                                    .Text(FText::FromString(TEXT("链接到骨骼网格体")))
                                    .OnClicked(this, &DoodleVariantCompoundWidget::OnLinkSkeletalMesh)
                            ]
                    ]
                    + SVerticalBox::Slot()
                    .Padding(0.f,0.f,0.f,15.f)
                    .VAlign(VAlign_Top)
            ]
            + SVerticalBox::Slot()
            [
                SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
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
                                    //------------------------------
                                    NowVaraint = *inSelectItem;
                                    if (CurrentObject && CurrentObject->AllVaraint.Num() > 0)
                                        OnVariantChange.ExecuteIfBound(CurrentObject->AllVaraint[NowVaraint]);
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
                            .OnContextMenuOpening_Lambda([this]() 
                            {
                                if (CurrentObject && CurrentObject->AllVaraint.Num() > 1) 
                                {
                                    FUIAction ActionDelete(FExecuteAction::CreateRaw(this, &DoodleVariantCompoundWidget::OnVariantDelete), FCanExecuteAction());
                                    FMenuBuilder MenuBuilder(true, false);
                                    MenuBuilder.AddMenuSeparator();
                                    MenuBuilder.AddMenuEntry(FText::FromString(TEXT("删除")), FText::FromString(TEXT("删除变体")),
                                        FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Delete")), ActionDelete);
                                    return MenuBuilder.MakeWidget();
                                }
                                return SNullWidget::NullWidget;
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
                    + SHorizontalBox::Slot()
                    .HAlign(HAlign_Fill)
                    .FillWidth(1.5f)
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
                                + SHeaderRow::Column(TEXT("Number")).DefaultLabel(FText::FromString(TEXT("插槽-材质")))
                            )
                    ]
            ]
       
    ];
  // clang-format on
    
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
            FString TheVaraint = *InItem;
            OnVariantAttach(TheVaraint);
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
            CurrentObject->AllVaraint.Remove(OldName);    
            CurrentObject->AllVaraint.Shrink();
            CurrentObject->AllVaraint.Add(NewName, L_Data);
            if (NowVaraint.Equals(OldName))
            {
                NowVaraint = NewName;
            }
            CurrentObject->Modify();
        }
    }
    Items.Empty();
    for (auto& e : CurrentObject->AllVaraint)
    {
        TSharedPtr<FString> f = MakeShared<FString>(e.Key);
        Items.Add(f);
    }
    ThisListView->RequestListRefresh();
    //-------------
    for (TSharedPtr<FString> Item : Items)
    {
        if (*Item == NewName)
        {
            ThisListView->Private_SetItemSelection(Item, true);
            break;
        }
    }
}

TSharedRef<ITableRow> DoodleVariantCompoundWidget::MaterialListOnGenerateRow(TSharedPtr<FMaterialItemData> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(SMaterialItemTableRow, OwnerTable)
        .InItem(InItem)
        .GetCheckState_Lambda([this, InItem]()
        {
            if (MaterialGetCheckState.IsBound())
                return MaterialGetCheckState.Execute(InItem->Index);
            else
                return ECheckBoxState::Undetermined;
        })
        .OnCheckStateChanged_Lambda([this, InItem](ECheckBoxState state)
        {
            OnMaterialCheckStateChanged.ExecuteIfBound(state, InItem->Index);
        })
        .OnObjectChanged_Lambda([this, InItem](FAssetData AssetData,FName SlotName)
        {
            if (CurrentObject)
            {
                int index = InItem->Index;
                FVariantInfo Arr = CurrentObject->AllVaraint[NowVaraint];
                UObject* u = AssetData.GetAsset();
                TObjectPtr<UMaterialInterface> ui = Cast<UMaterialInterface>(AssetData.GetAsset());
                if (ui) 
                {
                    Arr.Variants[index] = FSkeletalMaterial(ui,true,false,SlotName);
                    CurrentObject->AllVaraint[NowVaraint] = Arr;
                    MaterialItems[index]->Material = Arr.Variants[index].MaterialInterface;
                    CurrentObject->Modify();
                }
                //----------------
                //NowVaraint = *InItem;
                if (CurrentObject && CurrentObject->AllVaraint.Num() > 0)
                    OnVariantChange.ExecuteIfBound(CurrentObject->AllVaraint[NowVaraint]);
            }
        });
}
//-------------------------------------------
FReply DoodleVariantCompoundWidget::OnLoadAllVariant()
{
    FContentBrowserModule& ContentBrowserModle = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    TArray<FAssetData> SelectedAsset;
    ContentBrowserModle.Get().GetSelectedAssets(SelectedAsset);
    if (SelectedAsset.Num() <= 0|| !SelectedAsset[0].GetClass()->IsChildOf<USkeletalMesh>())
    {
        FText  DialogText = FText::FromString(TEXT("请先在内容浏览器中，选择一个骨骼网格体。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    FAssetData SelectedData = SelectedAsset[0];
    //----------------------------------------------------
    UObject* MeshObj = SelectedData.GetAsset();
    USkeletalMesh* L_Mesh = Cast<USkeletalMesh>(MeshObj);
    UDoodleVariantAssetUserData* UserData = L_Mesh->GetAssetUserData<UDoodleVariantAssetUserData>();
    if (UserData && UserData->VariantObj)
    {
        TArray<FString> OutKeys;
        CurrentObject = UserData->VariantObj;
        CurrentObject->AllVaraint.GetKeys(OutKeys);
        if (OutKeys.Num() > 0)
            NowVaraint = OutKeys[0];
        if (!UserData->VariantObj->Mesh) 
        {
            UserData->VariantObj->Mesh = L_Mesh;
            L_Mesh->AddAssetUserData(UserData);
        }
    }
    else
    {
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        FString out_name = MakeUniqueObjectName(L_Mesh, UDoodleVariantObject::StaticClass(), SelectedData.AssetName).ToString();
        FString PackageName = SelectedData.PackagePath.ToString() + "/" + out_name;
        const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
        UPackage* package = CreatePackage(*PackageName);
        package->MarkPackageDirty();
        UDoodleVariantFactory* MyFactory = NewObject<UDoodleVariantFactory>(UDoodleVariantFactory::StaticClass());
        FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
        UObject* object = AssetToolsModule.Get().CreateAsset(out_name, PackagePath, UDoodleVariantObject::StaticClass(), MyFactory);
        CurrentObject = Cast<UDoodleVariantObject>(object);
        //-------------------------
        UserData = NewObject<UDoodleVariantAssetUserData>(L_Mesh, NAME_None, RF_NoFlags);
        FAssetData variant_date = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(CurrentObject));
        UserData->VariantObj = CurrentObject;
        L_Mesh->AddAssetUserData(UserData);
        //----------------------------
        AssetRegistryModule.Get().AssetCreated(CurrentObject);
    }
    CurrentObject->Mesh = L_Mesh;
    if (CurrentObject->AllVaraint.Num() <= 0)
    {
        TArray<FSkeletalMaterial> trangeMat = L_Mesh->GetMaterials();
        for (int M = 0; M < trangeMat.Num(); M++) {
            trangeMat[M] = L_Mesh->GetMaterials()[M];
        }
        FVariantInfo Info;
        Info.Variants = trangeMat;
        NowVaraint = TEXT("default");
        CurrentObject->AllVaraint.Add(NowVaraint, Info);
    }
    //-------------
    NameText->SetColorAndOpacity(FLinearColor::White);
    NameText->SetText(FText::FromString(CurrentObject->Mesh->GetPathName()));
    Items.Empty();
    for (auto& e : CurrentObject->AllVaraint)
    {
        TSharedPtr<FString> Str = MakeShared<FString>(e.Key);
        Items.Add(Str);
    }
    ThisListView->RequestListRefresh();
    //-------------
    for (TSharedPtr<FString> Item : Items)
    {
        if (*Item == NowVaraint)
        {
            ThisListView->SetItemSelection(Item, true);
            break;
        }
    }
    SetVariantInfo(NowVaraint);
    ButtonLoadVariant->SetVisibility(EVisibility::Hidden);
	return FReply::Handled();
}

FReply DoodleVariantCompoundWidget::OnLinkSkeletalMesh() 
{
    FContentBrowserModule& ContentBrowserModle = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    TArray<FAssetData> SelectedAsset;
    ContentBrowserModle.Get().GetSelectedAssets(SelectedAsset);
    if (SelectedAsset.Num() <= 0|| !SelectedAsset[0].GetClass()->IsChildOf<USkeletalMesh>())
    {
        FText  DialogText = FText::FromString(TEXT("请先在内容浏览器中，选择一个骨骼网格体。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }
    FAssetData SelectedData = SelectedAsset[0];
    //----------------------------------------------------
    UObject* MeshObj = SelectedData.GetAsset();
    USkeletalMesh* L_Mesh = Cast<USkeletalMesh>(MeshObj);
    if (CurrentObject) 
    {
        if (!CurrentObject->Mesh)
        {
            if (CurrentObject->AllVaraint.Num() <= 0)
            {
                CurrentObject->Mesh = L_Mesh;
                TArray<FSkeletalMaterial> trangeMat = L_Mesh->GetMaterials();
                for (int M = 0; M < trangeMat.Num(); M++) {
                    trangeMat[M] = L_Mesh->GetMaterials()[M];
                }
                FVariantInfo Info;
                Info.Variants = trangeMat;
                NowVaraint = TEXT("default");
                CurrentObject->AllVaraint.Add(NowVaraint, Info);
            }
            else
            {
                if (L_Mesh->GetMaterials().Num() != CurrentObject->AllVaraint.begin().Value().Variants.Num())
                {
                    FNotificationInfo L_Info{ FText::FromString(TEXT("链接失败，网格体不匹配")) };
                    L_Info.FadeInDuration = 2.0f;  // 
                    L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Error"));
                    FSlateNotificationManager::Get().AddNotification(L_Info);
                    return FReply::Handled();
                }
                CurrentObject->Mesh = L_Mesh;
                TArray<FSkeletalMaterial> Materials = L_Mesh->GetMaterials();
                for (TPair<FString, FVariantInfo> Data : CurrentObject->AllVaraint)
                {
                    FVariantInfo Info = Data.Value;
                    for (int M = 0; M < Info.Variants.Num(); M++)
                    {
                        if (M < Materials.Num() && Info.Variants[M].MaterialSlotName.IsNone())
                            Info.Variants[M].MaterialSlotName = Materials[M].MaterialSlotName;
                    }
                    CurrentObject->AllVaraint[Data.Key] = Info;
                }
                //------------------------
                UDoodleVariantAssetUserData* UserData = NewObject<UDoodleVariantAssetUserData>(L_Mesh, NAME_None, RF_NoFlags);
                UserData->VariantObj = CurrentObject;
                L_Mesh->AddAssetUserData(UserData);
                //-----------------------
                TArray<FString> OutKeys;
                CurrentObject->AllVaraint.GetKeys(OutKeys);
                if (OutKeys.Num() > 0)
                    NowVaraint = OutKeys[0];
                ButtonLinkMesh->SetVisibility(EVisibility::Hidden);
                //------------------
                CurrentObject->Modify();
            }
        }
        //-------------
        NameText->SetColorAndOpacity(FLinearColor::White);
        NameText->SetText(FText::FromString(CurrentObject->Mesh->GetPathName()));
        Items.Empty();
        for (auto& e : CurrentObject->AllVaraint)
        {
            TSharedPtr<FString> Str = MakeShared<FString>(e.Key);
            Items.Add(Str);
        }
        ThisListView->RequestListRefresh();
        //-------------
        for (TSharedPtr<FString> Item : Items)
        {
            if (*Item == NowVaraint)
            {
                ThisListView->SetItemSelection(Item, true);
                break;
            }
        }
        SetVariantInfo(NowVaraint);
    }
    return FReply::Handled();
}

void DoodleVariantCompoundWidget::SetSetVariantData(UDoodleVariantObject* obj)
{
    CurrentObject = obj;
    //------------------------
    if (CurrentObject->Mesh) 
    {
        NameText->SetColorAndOpacity(FLinearColor::White);
        NameText->SetText(FText::FromString(CurrentObject->Mesh->GetPathName()));
        ButtonLinkMesh->SetVisibility(EVisibility::Hidden);
        //-----------------
        TArray<FSkeletalMaterial> Materials = CurrentObject->Mesh->GetMaterials();
        for (TPair<FString, FVariantInfo> Data : CurrentObject->AllVaraint)
        {
            FVariantInfo Info = Data.Value;
            for (int M=0;M<Info.Variants.Num();M++) 
            {
                if(M< Materials.Num() && Info.Variants[M].MaterialSlotName.IsNone())
                    Info.Variants[M].MaterialSlotName = Materials[M].MaterialSlotName;
            }
            CurrentObject->AllVaraint[Data.Key] = Info;
        }
        CurrentObject->Modify();
    }
    else
    {
        FNotificationInfo Info(FText::FromString(TEXT("骨骼网格体已丢失，请重新链接")));
        Info.FadeInDuration = 2.0f;  
        Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Error"));
        FSlateNotificationManager::Get().AddNotification(Info);
        //--------------
        NameText->SetColorAndOpacity(FLinearColor::Red);
        NameText->SetText(FText::FromString(TEXT("骨骼网格体已丢失，请重新链接...")));
        ButtonLinkMesh->SetVisibility(EVisibility::Visible);
    }
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
    if (OutKeys.Num() > 0) 
    {
        NowVaraint = OutKeys[0];
        SetVariantInfo(NowVaraint);
    }
    //-------------
    for (TSharedPtr<FString> Item : Items)
    {
        if (*Item == NowVaraint)
        {
            ThisListView->SetItemSelection(Item, true);
            break;
        }
    }
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
        //-------------
        for (TSharedPtr<FString> Item : Items)
        {
            if (*Item == NowVaraint)
            {
                ThisListView->SetItemSelection(Item, true);
                break;
            }
        }
        SetVariantInfo(L_Name.ToString());
        CurrentObject->Modify();
    }
      
    return FReply::Handled();
}

void DoodleVariantCompoundWidget::OnVariantDelete()
{
    const TArray<TSharedPtr<FString>> Selection = ThisListView->GetSelectedItems();
    if (!Selection.IsEmpty())
    {
        if (CurrentObject->AllVaraint.Num() > 1)
        {
            for (TSharedPtr<FString> L_Key : Selection)
            {
                if (CurrentObject->AllVaraint.Contains(*L_Key))
                {
                    CurrentObject->AllVaraint.Remove(*L_Key);
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
            if (OutKeys.Num() > 0) {
                NowVaraint = OutKeys[0];
                SetVariantInfo(NowVaraint);
            }
            CurrentObject->Modify();
            //-------------
            for (TSharedPtr<FString> Item : Items)
            {
                if (*Item == NowVaraint)
                {
                    ThisListView->Private_SetItemSelection(Item, true);
                    break;
                }
            }
        }
    }
}

FReply DoodleVariantCompoundWidget::OnVariantAttach(FString TheVaraint)
{
    if (!CurrentObject)
    {
        FText DialogText = {};
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
        FText DialogText = {};
        DialogText = FText::FromString(TEXT("请先在关卡中，选择粘贴目标。"));
        FMessageDialog::Open(EAppMsgType::Ok, DialogText);
        return FReply::Handled();
    }

    UObject* L_Target = L_Selects->GetSelectedObject(0);
    ASkeletalMeshActor* L_Skin = Cast<ASkeletalMeshActor>(L_Target);
    TArray<FSkeletalMaterial> L_List = CurrentObject->AllVaraint[TheVaraint].Variants;
    for (int i = 0;i < L_List.Num();i++)
    {
        FName SlotName = L_List[i].MaterialSlotName;
        int32 Index = L_Skin->GetSkeletalMeshComponent()->GetMaterialIndex(SlotName);
        if(Index!=-1)
        L_Skin->GetSkeletalMeshComponent()->SetMaterial(Index, L_List[i].MaterialInterface);
    }
    L_Skin->GetSkeletalMeshComponent()->PostApplyToComponent();
    //-----------------------
    return FReply::Handled();
}

TSharedRef<SDockTab> DoodleVariantCompoundWidget::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(DoodleVariantCompoundWidget)]; 
}
