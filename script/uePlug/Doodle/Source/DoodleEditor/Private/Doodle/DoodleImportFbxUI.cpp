#include "DoodleImportFbxUI.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Input/SDirectoryPicker.h"
#define LOCTEXT_NAMESPACE "SDoodleImportFbxUI"
const FName SDoodleImportFbxUI::Name{TEXT("DoodleImportFbxUI")};

void SDoodleImportFbxUI::Construct(const FArguments& Arg) {
  const FSlateFontInfo Font = FEditorStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));

  // clang-format off
  ChildSlot
  [
    SNew(SBorder)
      .BorderBackgroundColor(FLinearColor(0.3, 0.3, 0.3, 0.0f))
      .BorderImage(new FSlateBrush())
      [
        SNew(SVerticalBox) 
        + SVerticalBox::Slot()
        .AutoHeight()
        .VAlign(VAlign_Center)
        .Padding(2.0f)
        [
          // 扫描的文件目录
          SNew(SHorizontalBox)
          +SHorizontalBox::Slot()
          .FillWidth(1.0f)
          [
            SNew(STextBlock)
            .Text(LOCTEXT("BinaryPathLabel", "search path"))
            .ToolTipText(LOCTEXT("BinaryPathLabel_Tooltip", "search path"))
            .Font(Font)
          ]
          +SHorizontalBox::Slot()
          .FillWidth(2.0f)
          [
            SNew(SDirectoryPicker)
            .OnDirectoryChanged_Raw(this,&SDoodleImportFbxUI::SearchPath)
          ]
        ]

      ]



    ];
  // clang-format on
  // SNew(SDirectoryPicker)
}
void SDoodleImportFbxUI::SearchPath(const FString& in) {
}
void SDoodleImportFbxUI::AddReferencedObjects(FReferenceCollector& collector) {}

void SDoodleImportFbxUI::CreateDoodleUI(FMenuBuilder& MenuBuilder) {
}
TSharedRef<SDockTab> SDoodleImportFbxUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SDoodleImportFbxUI)  // 这里创建我们自己的界面
  ];
}
#undef LOCTEXT_NAMESPACE