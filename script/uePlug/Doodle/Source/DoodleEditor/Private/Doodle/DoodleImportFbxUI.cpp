#include "DoodleImportFbxUI.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Input/SDirectoryPicker.h"

const FName SDoodleImportFbxUI::Name{TEXT("DoodleImportFbxUI")};

void SDoodleImportFbxUI::Construct(const FArguments& Arg) {
  // clang-format off
  ChildSlot[
    SNew(SBorder)
      .BorderBackgroundColor(FLinearColor(0.3, 0.3, 0.3, 1))
      .BorderImage(new FSlateBrush())
      [
        SNew(SVerticalBox) +
        // 扫描的文件目录
         SVerticalBox::Slot()
        .AutoHeight()
        .VAlign(VAlign_Center)
        .Padding(2.0f)
          [
            SNew(SDirectoryPicker)

          ]
      ]



    ];
  // clang-format on
}

void SDoodleImportFbxUI::AddReferencedObjects(FReferenceCollector& collector) {}

void SDoodleImportFbxUI::CreateDoodleUI(FMenuBuilder& MenuBuilder) {
}
TSharedRef<SDockTab> SDoodleImportFbxUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[
      // Put your tab content here!
      SNew(SBox)
          .HAlign(HAlign_Left)
          .VAlign(VAlign_Top)[SNew(SDoodleImportFbxUI)  // 这里创建我们自己的界面
  ]];
}