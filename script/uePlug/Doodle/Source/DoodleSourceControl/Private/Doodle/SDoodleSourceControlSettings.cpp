// Copyright Epic Games, Inc. All Rights Reserved.

#include "SDoodleSourceControlSettings.h"
#include "Fonts/SlateFontInfo.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SFilePathPicker.h"
#include "EditorDirectories.h"
#include "EditorStyleSet.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "SourceControlOperations.h"

#include "Doodle/SourceControlProvider.h"
#include "DoodleSourceControl.h"

#define LOCTEXT_NAMESPACE "SDoodleSourceControlSettings"
void SDoodleSourceControlSettings::Construct(const FArguments& InArgs) {
  const FSlateFontInfo Font  = FEditorStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));
  const FText FileFilterType = LOCTEXT("Executables", "Executables");
#if PLATFORM_WINDOWS
  const FString FileFilterText = FString::Printf(TEXT("%s (*.uproject)|*.uproject"), *FileFilterType.ToString());
#else
  const FString FileFilterText = FString::Printf(TEXT("%s"), *FileFilterType.ToString());
#endif
  FText ReadmeContent = FText::FromString(FString(TEXT("# ")) + FApp::GetProjectName() + "\n\nDeveloped with Unreal Engine 4\n");
  // clang-format off
  ChildSlot
  [SNew(SBorder)
    .BorderImage( FEditorStyle::GetBrush("DetailsView.CategoryBottom"))
    .Padding(FMargin(0.0f, 3.0f, 0.0f, 0.0f))
    [
        // 根目录部件
        SNew(SVerticalBox) 
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        .VAlign(VAlign_Center)
        .Expose(AddFileRootWidget_Expose) // 此处槽获得下方的垂直排列小部件
        [
            SNew(SVerticalBox) // 垂直排列小部件
            // 添加一条记录
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2.0f)
            .VAlign(VAlign_Center)
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                SNew(STextBlock)
                .Text(LOCTEXT("BinaryPathLabel", "Doodle DataBase Path"))
                .ToolTipText(LOCTEXT("BinaryPathLabel_Tooltip", "Path to Doodle DataBase binary"))
                .Font(Font)
                ]
                +SHorizontalBox::Slot()
                .FillWidth(2.0f)
                [
                    SNew(SFilePathPicker)
                    .BrowseButtonImage(FEditorStyle::GetBrush("PropertyWindow.Button_Ellipsis"))
                    .BrowseButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
                    .BrowseButtonToolTip(LOCTEXT("BinaryPathLabel_Tooltip", "Path to Doodle DataBase binary"))
                    .BrowseDirectory(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_OPEN))
                    .BrowseTitle(LOCTEXT("BinaryPathBrowseTitle", "File picker..."))
                    .FilePath_Lambda([this]()->FString{
                      return this->PathToRepositoryRoot;
                    })
                    .FileTypeFilter(FileFilterText)
                    .OnPathPicked_Lambda([this](const FString& PickedPath){
                      this->SetPathToRepositoryRoot(PickedPath);

                    })
                ]
            ]
        ]

        // 添加
        +SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(2.0f)
        .VAlign(VAlign_Center)
        [
            SNew(SHorizontalBox)
            // 清除所有的跟路径
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(SButton)
                .Text(LOCTEXT("Clear","Clear"))
                .ToolTipText(LOCTEXT("Clear_Tip","Clear All Root Path"))
                
            ]
            // 添加一条跟路径
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(SButton)
                .Text(LOCTEXT("Add","Add"))
                .ToolTipText(LOCTEXT("Add_Tip","Add  Root Path"))
                .OnClicked_Lambda([this](){ this->AddNewFileRoot(); return FReply::Handled(); })
            ]
        ]
    ]
  ];
  // clang-format on
}

void SDoodleSourceControlSettings::AddNewFileRoot() {
  const FSlateFontInfo Font  = FEditorStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));
  const FText FileFilterType = LOCTEXT("Executables", "Executables");
#if PLATFORM_WINDOWS
  const FString FileFilterText = FString::Printf(TEXT("%s (*.exe)|*.exe"), *FileFilterType.ToString());
#else
  const FString FileFilterText = FString::Printf(TEXT("%s"), *FileFilterType.ToString());
#endif
  if (AddFileRootWidget_Expose->GetWidget()->GetType() == ("SVerticalBox")) {
    auto L_ = static_cast<SHorizontalBox*>(&AddFileRootWidget_Expose->GetWidget().Get());
    L_->AddSlot()
        // clang-format off
    .FillWidth(1.0f)
    .AttachWidget(
        SNew(SFilePathPicker)
        .BrowseButtonImage(FEditorStyle::GetBrush("PropertyWindow.Button_Ellipsis"))
        .BrowseButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
        .BrowseButtonToolTip(LOCTEXT("BinaryPathLabel_Tooltip", "Path to Doodle DataBase binary"))
        .BrowseDirectory(FEditorDirectories::Get()
        .GetLastDirectory(ELastDirectory::GENERIC_OPEN))
        .BrowseTitle(LOCTEXT("BinaryPathBrowseTitle", "File picker...")
        )
        // .FilePath_Lambda([]()->FString{return FString{};})
        .FileTypeFilter(FileFilterText)
        .OnPathPicked_Lambda([this](const FString& PickedPath) {})
    );

    // clang-format on
  }
}

void SDoodleSourceControlSettings::SetPathToRepositoryRoot(const FString& InPath) {
  FString LP = FPaths::ConvertRelativePathToFull(FPaths::GetPath(InPath));
  UE_LOG(LogTemp, Log, TEXT("获取路径 %s"), *(LP));

  /// 测试一些路径函数
  // FString LTmp = LP / "tset" / "dddddd";
  // LTmp.RemoveFromStart(LP);
  // UE_LOG(LogTemp, Log, TEXT("FPaths::ConvertRelativePathToFull %s"), *LTmp);

  FDoodleSourceControlModule& LModle        = FModuleManager::LoadModuleChecked<FDoodleSourceControlModule>("DoodleSourceControl");
  LModle.GetProvider().PathToRepositoryRoot = LP;
  this->PathToRepositoryRoot                = LP;
}

SDoodleSourceControlSettings::~SDoodleSourceControlSettings() = default;

#undef LOCTEXT_NAMESPACE