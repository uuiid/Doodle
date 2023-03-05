#include "CreateCharacter_Factory.h"

#include "Doodle/CreateCharacter/CoreData/DoodleCreateCharacterConfig.h"
#include "EditorStyleSet.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "SCreateCharacterDialog"

class SCreateCharacterDialog : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SCreateCharacterDialog)
      : _CreateCharacterFactory() {}

  SLATE_ARGUMENT(UCreateCharacter*, CreateCharacterFactory)

  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs) {
    CreateCharacterFactory = InArgs._CreateCharacterFactory;

    // clang-format off
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
              .OnClicked_Lambda([this](){	CloseDialog(true); return FReply::Handled(); })
              .Text(LOCTEXT("CreateMotionFieldOk", "OK"))
            ]
            + SUniformGridPanel::Slot(1, 0)
            [
              SNew(SButton)
              .HAlign(HAlign_Center)
              .ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
              .OnClicked_Lambda([this](){	CloseDialog(); return FReply::Handled(); })
              .Text(LOCTEXT("CreateMotionFieldCancel", "Cancel"))
            ]
          ]
        ]
      ]
    ];
    // clang-format on

    MakeSkeletonPicker();
  }

  bool Show() {
    // clang-format off
    TSharedRef<SWindow> Window = SNew(SWindow)
      .Title(LOCTEXT("Show", "Create Character Config"))
      .ClientSize(FVector2D{400,700})
      .SupportsMinimize(false)
      .SupportsMaximize(false)
      [
        SharedThis(this) 
      ];
    // clang-format on

    PickerWindow = Window;
    GEditor->EditorAddModalWindow(Window);
    return bOkClicked;
  }

 private:
  void MakeSkeletonPicker() {
    FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

    FAssetPickerConfig AssetPickerConfig;
    AssetPickerConfig.Filter.ClassPaths.Add(FTopLevelAssetPath{USkeletalMesh::StaticClass()->GetPathName()});
    AssetPickerConfig.OnAssetSelected       = FOnAssetSelected::CreateSP(this, &SCreateCharacterDialog::OnSkeletonSelected);
    AssetPickerConfig.OnShouldFilterAsset   = FOnShouldFilterAsset::CreateSP(this, &SCreateCharacterDialog::FilterSkeletonBasedOnParentClass);
    AssetPickerConfig.bAllowNullSelection   = true;
    AssetPickerConfig.InitialAssetViewType  = EAssetViewType::Column;
    AssetPickerConfig.InitialAssetSelection = TargetSkeletonMesh;

    SkeletonContainer->ClearChildren();
    // clang-format off
    SkeletonContainer->AddSlot()
    .AutoHeight()
    [
      SNew(STextBlock)
      .Text(LOCTEXT("TargetSkeletonMesh", "Target Skeleton Mesh:"))
      .ShadowOffset(FVector2D(1.0f, 1.0f))
    ];
    SkeletonContainer->AddSlot()
    [
      ContentBrowserModule.Get()
      .CreateAssetPicker(AssetPickerConfig)
    ];
    // clang-format on
  }

  void OnSkeletonSelected(const FAssetData& AssetData) {
    TargetSkeletonMesh = AssetData;
  }
  void CloseDialog(bool bWasPicked = false) {
    bOkClicked = bWasPicked;
    if (CreateCharacterFactory.IsValid()) {
      CreateCharacterFactory->SkeletalMesh = Cast<USkeletalMesh>(TargetSkeletonMesh.GetAsset());
    }
    if (PickerWindow.IsValid()) {
      PickerWindow.Pin()->RequestDestroyWindow();
    }
  }

  bool FilterSkeletonBasedOnParentClass(const FAssetData& AssetData) {
    return false;  // !CanCreateMotionField(AssetData, ParentClass.Get());
  }

  // 骨骼网格体列表
  TSharedPtr<SVerticalBox> SkeletonContainer;

  // 创建工厂
  TWeakObjectPtr<UCreateCharacter> CreateCharacterFactory;
  // 选择的骨骼网格体资产
  FAssetData TargetSkeletonMesh;
  // 点击 OK
  bool bOkClicked;
  // 父窗口
  TWeakPtr<SWindow> PickerWindow;
};

#undef LOCTEXT_NAMESPACE

UCreateCharacter::UCreateCharacter(const FObjectInitializer& ObjectInitializer) {
  bCreateNew     = true;
  bEditAfterNew  = true;
  SupportedClass = UDoodleCreateCharacterConfig::StaticClass();
}

bool UCreateCharacter::ConfigureProperties() {
  TSharedRef<SCreateCharacterDialog> Dialog = SNew(SCreateCharacterDialog).CreateCharacterFactory(this);
  return Dialog->Show();
}

UObject* UCreateCharacter::FactoryCreateNew(
    UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn,
    FName CallingContext
) {
  UDoodleCreateCharacterConfig* L_Ass = NewObject<UDoodleCreateCharacterConfig>(InParent, Class, Name, Flags | RF_Transactional);

  if (L_Ass && SkeletalMesh) {
    L_Ass->SetSkeletalMesh(SkeletalMesh);
    return L_Ass;
  }

  return nullptr;
}

UObject* UCreateCharacter::FactoryCreateNew(
    UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn
) {
  return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}
