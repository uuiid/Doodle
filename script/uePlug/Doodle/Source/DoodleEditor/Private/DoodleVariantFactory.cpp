// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleVariantFactory.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "DoodleVariantAssetUserData.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Engine/SkinnedAssetCommon.h"

class SCreateVariantDialog : public SCompoundWidget {
public:
    SLATE_BEGIN_ARGS(SCreateVariantDialog)
        {}
    SLATE_END_ARGS()
//--------------
private:
    TSharedPtr<SVerticalBox> SkeletonContainer;
    TWeakPtr<SWindow> PickerWindow;
public:
    FAssetData MeshAssetData;

    void Construct(const FArguments& InArgs) 
    {
        ChildSlot
        [
            SNew(SBorder)
                .Visibility(EVisibility::Visible)
                .BorderImage(FAppStyle::GetBrush("Menu.Background"))
                [
                    SNew(SBox)
                        .Visibility(EVisibility::Visible)
                        .WidthOverride(500.0f)
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
                                            SAssignNew(SkeletonContainer, SVerticalBox)
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
                                                .OnClicked_Lambda([this]() {	
                                                    if (PickerWindow.IsValid()) {
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
                                                .OnClicked_Lambda([this]() {
                                                MeshAssetData = nullptr;
                                                    if (PickerWindow.IsValid()) {
                                                        PickerWindow.Pin()->RequestDestroyWindow();
                                                    }
                                                return FReply::Handled(); })
                                                .Text(FText::FromString(TEXT("Cancel")))
                                        ]
                                ]
                        ]
                ]
        ];
        //-----------------------------------------------------
        FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

        FAssetPickerConfig AssetPickerConfig;
        AssetPickerConfig.Filter.ClassPaths.Add(FTopLevelAssetPath{ USkeletalMesh::StaticClass()->GetPathName() });
        AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateLambda([this](const FAssetData& AssetData)
        {
            MeshAssetData = AssetData;
        });
        AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateLambda([](const FAssetData& AssetData) {return false;});
        AssetPickerConfig.bAllowNullSelection = true;
        AssetPickerConfig.InitialAssetViewType = EAssetViewType::Column;
        AssetPickerConfig.InitialAssetSelection = MeshAssetData;

        SkeletonContainer->ClearChildren();
        SkeletonContainer->AddSlot()
        .AutoHeight()
        [
            SNew(STextBlock)
                .Text(FText::FromString(TEXT("Target Skeleton Mesh:")))
                .ShadowOffset(FVector2D(1.0f, 1.0f))
        ];
        SkeletonContainer->AddSlot()
        [
            ContentBrowserModule.Get()
                .CreateAssetPicker(AssetPickerConfig)
        ];
    }

    void Show() {
        TSharedRef<SWindow> Window = SNew(SWindow)
            .Title(FText::FromString(TEXT("Create Variant Config")))
            .ClientSize(FVector2D{ 600,700 })
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

UDoodleVariantFactory::UDoodleVariantFactory() {
    bCreateNew = true;
    bEditAfterNew = true;
    SupportedClass = UDoodleVariantObject::StaticClass();
}

bool UDoodleVariantFactory::ConfigureProperties() 
{
    TSharedRef<SCreateVariantDialog> Dialog = SNew(SCreateVariantDialog);
    Dialog->Show();
    if (Dialog.Get().MeshAssetData!= nullptr)
    {
        MeshAssetData = Dialog.Get().MeshAssetData;
        USkeletalMesh* L_Mesh = Cast<USkeletalMesh>(MeshAssetData.GetAsset());
        UDoodleVariantAssetUserData* user_data = L_Mesh->GetAssetUserData<UDoodleVariantAssetUserData>();
        FAssetData variant_date;
        if (user_data && user_data->VariantObj)
        {
            FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
            variant_date = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(user_data->VariantObj));
            UObject* object = variant_date.ToSoftObjectPath().TryLoad();
            UDoodleVariantObject* uObject = Cast<UDoodleVariantObject>(object);
            //--------------
            FNotificationInfo L_Info{ FText::FromString(TEXT("创建失败...")) };
            L_Info.FadeInDuration = 2.0f;  // 
            L_Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Error"));
            L_Info.Text = FText::FromString(TEXT("创建失败，变体文件已经存在。"));
            FSlateNotificationManager::Get().AddNotification(L_Info);
            //-------------------------------------------
            FContentBrowserModule& ContentBrowserModle = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
            TArray<UObject*> Objects;
            Objects.Add(uObject);
            ContentBrowserModle.Get().SyncBrowserToAssets(Objects);
            return false;
        }
        else
        {
            NewVaraint = L_Mesh->GetName() + "_Variant";
            return true;
        }
    }
    return false;
}

FString UDoodleVariantFactory::GetDefaultNewAssetName() const {
  return NewVaraint;
};

UObject* UDoodleVariantFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    UDoodleVariantObject* uObject = NewObject<UDoodleVariantObject>(InParent, InClass, InName, Flags | RF_Transactional);
    if (MeshAssetData.IsValid()) 
    {
        USkeletalMesh* mesh = Cast<USkeletalMesh>(MeshAssetData.GetAsset());
        TArray<FSkeletalMaterial> trangeMat = mesh->GetMaterials();
       for (int m = 0; m < trangeMat.Num(); m++) {
           trangeMat[m] = mesh->GetMaterials()[m];
       }
       uObject->Mesh = mesh;
       FVariantInfo Info;
       Info.Variants = trangeMat;
       FString NowVaraint = TEXT("default");
       uObject->AllVaraint.Add(NowVaraint, Info);
       //-----------------------------
       UDoodleVariantAssetUserData* UserData = NewObject<UDoodleVariantAssetUserData>(mesh, NAME_None, RF_NoFlags);
       FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
       FAssetData variant_date = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(uObject));
       //UserData->variant_asset = variant_date;
       UserData->VariantObj = uObject;
       mesh->AddAssetUserData(UserData);
    }
    return uObject;
}
