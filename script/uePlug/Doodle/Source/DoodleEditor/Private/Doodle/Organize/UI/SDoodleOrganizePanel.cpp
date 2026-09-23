// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/UI/SDoodleOrganizePanel.h"

#include "Doodle/Organize/DoodleAssetOrganizer.h"
#include "Doodle/Organize/DoodleAssetPathUtils.h"
#include "Doodle/Organize/DoodleOrganizeNotifications.h"
#include "Doodle/Organize/DoodleOrganizeSettings.h"
#include "Doodle/Organize/DoodleReferenceAudit.h"
#include "Doodle/Organize/UI/SDoodleAssetPathRow.h"
#include "Doodle/Organize/UI/SDoodleDuplicateTextureRow.h"

#include "AssetThumbnail.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformProcess.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"

#define LOCTEXT_NAMESPACE "DoodleOrganizePanel"

void SDoodleOrganizePanel::Construct(const FArguments& InArgs)
{
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(64));

	// 输入框的初值只读一次设置 (原实现每帧都去 GConfig 取)
	const UDoodleOrganizeSettings& Settings = UDoodleOrganizeSettings::Get();
	TargetFolderNameEdit = Settings.TargetFolderName;
	CharacterFolderNameEdit = Settings.CharacterFolderName;
	SuffixEdit = Settings.Suffix;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			BuildTabBar()
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2.0f)
		[
			SAssignNew(TabSwitcher, SWidgetSwitcher)
			+ SWidgetSwitcher::Slot()[ BuildOrganizeTab() ]
			+ SWidgetSwitcher::Slot()[ BuildDedupeTab() ]
			+ SWidgetSwitcher::Slot()[ BuildRenameTab() ]
			+ SWidgetSwitcher::Slot()[ BuildTextureTab() ]
			+ SWidgetSwitcher::Slot()[ BuildCleanupTab() ]
			+ SWidgetSwitcher::Slot()[ BuildDiagnosticsTab() ]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			BuildReportBar()
		]
	];

	SetActiveTab(ETab::Organize);
	RefreshReportBar();
}

// ---------------------------------------------------------------------------
// 通用布局
// ---------------------------------------------------------------------------

TSharedRef<SWidget> SDoodleOrganizePanel::BuildSectionHeader(const FText& Text)
{
	return SNew(STextBlock)
		.Text(Text)
		.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 0.4f, 1.0f)));
}

TSharedRef<SWidget> SDoodleOrganizePanel::BuildLabeledTextBox(const FText& Label, FString& InOutValue,
	const FText& Tooltip)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(STextBlock).Text(Label)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SEditableTextBox)
				.Text_Lambda([&InOutValue]() { return FText::FromString(InOutValue); })
				.ToolTipText(Tooltip)
				.OnTextChanged_Lambda([&InOutValue](const FText& InText) { InOutValue = InText.ToString(); })
				.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type) { SaveEditableSettings(); })
		];
}

TSharedRef<SWidget> SDoodleOrganizePanel::BuildTabBar()
{
	static const TCHAR* TabNames[(int32)ETab::Count] =
	{
		TEXT("整理"),
		TEXT("重复贴图"),
		TEXT("批量重命名"),
		TEXT("贴图工具"),
		TEXT("目录清理"),
		TEXT("诊断"),
	};

	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < (int32)ETab::Count; ++Index)
	{
		const ETab Tab = (ETab)Index;
		Box->AddSlot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SCheckBox)
					.Style(FAppStyle::Get(), "ToolBar.ToggleButton")
					.IsChecked(this, &SDoodleOrganizePanel::GetTabCheckState, Tab)
					.OnCheckStateChanged(this, &SDoodleOrganizePanel::OnTabCheckStateChanged, Tab)
					[
						SNew(STextBlock).Text(FText::FromString(TabNames[Index]))
					]
			];
	}
	return Box;
}

void SDoodleOrganizePanel::SetActiveTab(ETab InTab)
{
	ActiveTab = InTab;
	if (TabSwitcher.IsValid())
	{
		TabSwitcher->SetActiveWidgetIndex((int32)InTab);
	}
}

ECheckBoxState SDoodleOrganizePanel::GetTabCheckState(ETab InTab) const
{
	return ActiveTab == InTab ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDoodleOrganizePanel::OnTabCheckStateChanged(ECheckBoxState NewState, ETab InTab)
{
	if (NewState == ECheckBoxState::Checked)
	{
		SetActiveTab(InTab);
	}
}

TSharedRef<SWidget> SDoodleOrganizePanel::BuildReportBar()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(ReportBarText, STextBlock)
					.Text(FText::FromString(TEXT("就绪")))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f, 0.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("查看明细")))
					.OnClicked_Lambda([this]()
					{
						DoodleOrganize::ShowMessage(FText::FromString(LastReport.ToDetailedText()));
						return FReply::Handled();
					})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f, 0.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("打开报告目录")))
					.OnClicked(this, &SDoodleOrganizePanel::OnOpenReportDirectory)
			]
		];
}

void SDoodleOrganizePanel::RefreshReportBar()
{
	if (ReportBarText.IsValid())
	{
		ReportBarText->SetText(LastReport.Operation.IsEmpty()
			? FText::FromString(TEXT("就绪"))
			: LastReport.ToSummaryText());
	}
}

void SDoodleOrganizePanel::SetDiagnosticsText(const FString& Text)
{
	if (DiagnosticsText.IsValid())
	{
		DiagnosticsText->SetText(FText::FromString(Text));
	}
}

// ---------------------------------------------------------------------------
// 各页
// ---------------------------------------------------------------------------

TSharedRef<SWidget> SDoodleOrganizePanel::BuildOrganizeTab()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildSectionHeader(FText::FromString(TEXT("按类型整理到指定文件夹")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			BuildLabeledTextBox(FText::FromString(TEXT("目标文件夹名")), TargetFolderNameEdit,
				FText::FromString(TEXT("相对 /Game, 例如 DoodleTemp")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(SButton)
				.Text(FText::FromString(TEXT("整理选中资源")))
				.ToolTipText(FText::FromString(TEXT("把内容浏览器里选中的资源按类型分类到 /Game/<目标文件夹>/<子目录>")))
				.OnClicked(this, &SDoodleOrganizePanel::OnOrganizeSelected)
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 8.0f)
		[
			SNew(SSeparator)
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildSectionHeader(FText::FromString(TEXT("按类型整理整个工程")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			BuildLabeledTextBox(FText::FromString(TEXT("角色名称")), CharacterFolderNameEdit,
				FText::FromString(TEXT("拼音, 例如 XiaoMing; 会整理到 /Game/Character/<角色名>/")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(SButton)
				.Text(FText::FromString(TEXT("整理所有资源")))
				.OnClicked(this, &SDoodleOrganizePanel::OnOrganizeAll)
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 6.0f)
		[
			SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
				.Text(FText::FromString(TEXT("分类规则 (地图 / Other 兜底 / 排除表 / 自动快照 / 自动校验) 都在 项目设置 -> Plugins -> Doodle 文件整理 里配置。")))
		]

		+ SVerticalBox::Slot().FillHeight(1.0f)[ SNullWidget::NullWidget ];
}

TSharedRef<SWidget> SDoodleOrganizePanel::BuildDedupeTab()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("查找所有重复贴图")))
					.OnClicked(this, &SDoodleOrganizePanel::OnFindDuplicates)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("一键删除重复贴图")))
					.ToolTipText(FText::FromString(TEXT("每组保留一个, 其余合并过去并删除")))
					.OnClicked(this, &SDoodleOrganizePanel::OnConsolidateAllDuplicates)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNullWidget::NullWidget ]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
				.Text(FText::FromString(TEXT("右键选中行 -> 重命名 可以就地改名; 双击行会在内容浏览器里定位。")))
		]

		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(2.0f)
		[
			SAssignNew(DupTree, STreeView<TSharedPtr<FDoodleDupRowItem>>)
				.TreeItemsSource(&DupRows)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SDoodleOrganizePanel::MakeDupRowWidget)
				.OnGetChildren(this, &SDoodleOrganizePanel::HandleGetChildrenForDupTree)
				.HighlightParentNodesForSelection(true)
				.OnMouseButtonDoubleClick(this, &SDoodleOrganizePanel::OnDupRowDoubleClick)
				.OnContextMenuOpening(this, &SDoodleOrganizePanel::OnDupTreeContextMenu)
				.OnSelectionChanged_Lambda([this](TSharedPtr<FDoodleDupRowItem> Item, ESelectInfo::Type)
				{
					NowSelectedDupRow = Item;
				})
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(FName(TEXT("DoodlePath")))
						.DefaultLabel(FText::FromString(TEXT("路径")))
						.FillWidth(0.8f)
					+ SHeaderRow::Column(FName(TEXT("DoodleAction")))
						.DefaultLabel(FText::FromString(TEXT("操作")))
						.FillWidth(0.2f)
				)
		];
}

TSharedRef<SWidget> SDoodleOrganizePanel::BuildRenameTab()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildSectionHeader(FText::FromString(TEXT("批量加 / 去后缀 (对内容浏览器选中项生效)")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			BuildLabeledTextBox(FText::FromString(TEXT("后缀")), SuffixEdit,
				FText::FromString(TEXT("不带下划线, 例如 low; 结果形如 T_rock_low")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("添加文件_后缀")))
					.OnClicked(this, &SDoodleOrganizePanel::OnAddSuffix)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("去除文件_后缀")))
					.ToolTipText(FText::FromString(TEXT("只剥离真正匹配的后缀; 旧版盲截行为可在项目设置里打开")))
					.OnClicked(this, &SDoodleOrganizePanel::OnRemoveSuffix)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNullWidget::NullWidget ]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 8.0f)
		[
			SNew(SSeparator)
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildSectionHeader(FText::FromString(TEXT("路径过长扫描 (>256, 会造成磁盘/打包路径问题)")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("查找路径过长资源")))
					.OnClicked(this, &SDoodleOrganizePanel::OnFindLongPaths)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("导出为 CSV")))
					.OnClicked(this, &SDoodleOrganizePanel::OnExportLongPaths)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNullWidget::NullWidget ]
		]

		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(2.0f)
		[
			SAssignNew(PathList, SListView<TSharedPtr<FDoodlePathRowItem>>)
				.ListItemsSource(&PathRows)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SDoodleOrganizePanel::MakePathRowWidget)
				.OnMouseButtonDoubleClick(this, &SDoodleOrganizePanel::OnPathRowDoubleClick)
		];
}

TSharedRef<SWidget> SDoodleOrganizePanel::BuildTextureTab()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildSectionHeader(FText::FromString(TEXT("引擎内置贴图本地化")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(SButton)
				.Text(FText::FromString(TEXT("获取材质引用的引擎内置贴图到本地目录")))
				.ToolTipText(FText::FromString(TEXT("复制到 /Game/<目标文件夹>/Tex/, 并把引用改指到本地副本")))
				.OnClicked(this, &SDoodleOrganizePanel::OnPullEngineTextures)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
				.Text(FText::FromString(TEXT("目标文件夹名取「整理」页的设置。原引擎贴图会先备份到 Tex/Backup/。")))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 8.0f)
		[
			SNew(SSeparator)
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildSectionHeader(FText::FromString(TEXT("贴图尺寸")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(SButton)
				.Text(FText::FromString(TEXT("重置所有贴图尺寸为 2 的幂")))
				.OnClicked(this, &SDoodleOrganizePanel::OnResizeTextures)
		]

		+ SVerticalBox::Slot().FillHeight(1.0f)[ SNullWidget::NullWidget ];
}

TSharedRef<SWidget> SDoodleOrganizePanel::BuildCleanupTab()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildSectionHeader(FText::FromString(TEXT("空目录清理")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(SButton)
				.Text(FText::FromString(TEXT("删除所有空文件夹")))
				.OnClicked(this, &SDoodleOrganizePanel::OnDeleteEmptyDirectories)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
				.Text(FText::FromString(TEXT("判定方式是「目录里没有任何文件, 且资产注册表在该包路径下也没有资产」; __ExternalActors__ / __ExternalObjects__ 目录不会被删。")))
		]

		+ SVerticalBox::Slot().FillHeight(1.0f)[ SNullWidget::NullWidget ];
}

TSharedRef<SWidget> SDoodleOrganizePanel::BuildDiagnosticsTab()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildSectionHeader(FText::FromString(TEXT("引用审计 (用来定位「整理后引用丢失」)")))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda([]()
				{
					return UDoodleOrganizeSettings::Get().bAutoSnapshotBeforeOrganize
						? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([](ECheckBoxState NewState)
				{
					UDoodleOrganizeSettings* Settings = GetMutableDefault<UDoodleOrganizeSettings>();
					Settings->bAutoSnapshotBeforeOrganize = (NewState == ECheckBoxState::Checked);
					Settings->SaveConfig();
				})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("整理前自动拍摄引用快照")))
				]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda([]()
				{
					return UDoodleOrganizeSettings::Get().bAutoVerifyAfterOrganize
						? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([](ECheckBoxState NewState)
				{
					UDoodleOrganizeSettings* Settings = GetMutableDefault<UDoodleOrganizeSettings>();
					Settings->bAutoVerifyAfterOrganize = (NewState == ECheckBoxState::Checked);
					Settings->SaveConfig();
				})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("操作后自动比对引用差异 (只比对被移动的包)")))
				]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda([]()
				{
					return UDoodleOrganizeSettings::Get().bAutoScanDanglingSoftRefs
						? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([](ECheckBoxState NewState)
				{
					UDoodleOrganizeSettings* Settings = GetMutableDefault<UDoodleOrganizeSettings>();
					Settings->bAutoScanDanglingSoftRefs = (NewState == ECheckBoxState::Checked);
					Settings->SaveConfig();
				})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("操作后自动扫描未保存包里的悬空软引用 (注册表盲区)")))
				]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
			[
				SNew(SButton).Text(FText::FromString(TEXT("拍摄快照")))
					.OnClicked(this, &SDoodleOrganizePanel::OnCaptureSnapshot)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
			[
				SNew(SButton).Text(FText::FromString(TEXT("与上次快照比较")))
					.OnClicked(this, &SDoodleOrganizePanel::OnCompareWithLastSnapshot)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
			[
				SNew(SButton).Text(FText::FromString(TEXT("查找悬空引用")))
					.OnClicked(this, &SDoodleOrganizePanel::OnFindDangling)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
			[
				SNew(SButton).Text(FText::FromString(TEXT("查找悬空软引用 (含未保存包)")))
					.ToolTipText(FText::FromString(TEXT("注册表看不到未保存的包; 软引用也不会跟着重命名走 —— 这是「整理后引用丢失」的主要来源")))
					.OnClicked(this, &SDoodleOrganizePanel::OnFindDanglingSoftRefs)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
			[
				SNew(SButton).Text(FText::FromString(TEXT("补修软引用 (用上次操作的移动表)")))
					.ToolTipText(FText::FromString(TEXT("把仍指向旧路径的软引用改写成新路径。引擎只修注册表知道的引用者, 未保存的包会被漏掉 —— 这个按钮补上那部分。改过的包会被标脏, 需要保存。")))
					.OnClicked(this, &SDoodleOrganizePanel::OnRetargetSoftRefs)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNullWidget::NullWidget ]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 2.0f)
		[
			SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
				.Text(FText::FromString(TEXT("控制台: Doodle.Organize.DumpReferences / Doodle.Organize.CompareSnapshots / Doodle.Organize.FindDangling")))
		]

		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(4.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(DiagnosticsText, STextBlock)
					.Text(FText::FromString(TEXT("(还没有诊断结果)")))
			]
		];
}

// ---------------------------------------------------------------------------
// 统一操作入口
// ---------------------------------------------------------------------------

void SDoodleOrganizePanel::RunOperation(const FText& OperationName, TFunctionRef<FDoodleOrganizeReport()> Operation)
{
	const UDoodleOrganizeSettings& Settings = UDoodleOrganizeSettings::Get();

	// 1) 操作前快照
	FString SnapshotFilename;
	if (Settings.bAutoSnapshotBeforeOrganize)
	{
		FDoodleReferenceAudit::FSnapshot Before;
		if (FDoodleReferenceAudit::Capture(DoodleOrganize::GetGameRootPath(), Before))
		{
			SnapshotFilename = FDoodleReferenceAudit::MakeSnapshotFilename(TEXT("before"));
			if (Before.Save(SnapshotFilename))
			{
				LastSnapshotFilename = SnapshotFilename;
			}
			else
			{
				SnapshotFilename.Reset();
			}
		}
	}

	// 2) 事务
	TUniquePtr<FScopedTransaction> Transaction;
	if (GEditor != nullptr)
	{
		Transaction = MakeUnique<FScopedTransaction>(OperationName);
	}

	// 3) 执行
	const double StartTime = FPlatformTime::Seconds();
	FDoodleOrganizeReport Report = Operation();
	Report.ElapsedSeconds = FPlatformTime::Seconds() - StartTime;

	// 4) 操作后引用校验
	FString ReportFile;
	if (Settings.bAutoVerifyAfterOrganize && !SnapshotFilename.IsEmpty() && Report.MovedPackages.Num() > 0)
	{
		FDoodleReferenceAudit::FSnapshot Before;
		FDoodleReferenceAudit::FSnapshot After;
		if (FDoodleReferenceAudit::FSnapshot::Load(SnapshotFilename, Before)
			&& FDoodleReferenceAudit::Capture(DoodleOrganize::GetGameRootPath(), After))
		{
			FDoodleReferenceAudit::FDiffOptions Options;
			Options.MoveMap = Report.MovedPackages;

			const FDoodleReferenceAudit::FDiffResult DiffResult = FDoodleReferenceAudit::Diff(Before, After, Options);
			LastDiffText = DiffResult.ToText(200);
			SetDiagnosticsText(LastDiffText);

			if (DiffResult.HasAnyChange())
			{
				ReportFile = FDoodleReferenceAudit::WriteReportFile(TEXT("organize"),
					FString::Printf(TEXT("%s\n\n%s"), *Report.ToDetailedText(), *LastDiffText));
				FDoodleReferenceAudit::WriteReportFile(TEXT("organize_diff_csv"), DiffResult.ToCsv());
			}

			if (DiffResult.HasRealLoss())
			{
				for (int32 Index = 0; Index < DiffResult.LostEdges.Num() && Index < 20; ++Index)
				{
					const FDoodleReferenceAudit::FDiffEntry& Entry = DiffResult.LostEdges[Index];
					Report.AddWarning(FString::Printf(TEXT("引用丢失: %s -> %s (%s)"),
						*Entry.Package.ToString(), *Entry.Dependency.ToString(), *Entry.Note));
				}
				Report.AddWarning(FString::Printf(TEXT("共 %d 条真·引用丢失, 详见诊断页与报告文件"), DiffResult.LostEdges.Num()));
			}
		}
	}

	// 4b) 未保存包里的悬空软引用 —— 注册表快照比对看不到的盲区, 也是最可能的丢失形态
	if (Settings.bAutoScanDanglingSoftRefs)
	{
		const TArray<FDoodleReferenceAudit::FDiffEntry> DanglingSoftRefs =
			FDoodleReferenceAudit::FindDanglingSoftReferences(DoodleOrganize::GetGameRootPath(),
				/*bDirtyPackagesOnly*/ true, Settings.MaxReportedEdges);

		if (DanglingSoftRefs.Num() > 0)
		{
			for (int32 Index = 0; Index < DanglingSoftRefs.Num() && Index < 20; ++Index)
			{
				Report.AddWarning(FString::Printf(TEXT("悬空软引用: %s -> %s"),
					*DanglingSoftRefs[Index].Package.ToString(),
					*DanglingSoftRefs[Index].Dependency.ToString()));
			}
			Report.AddWarning(FString::Printf(
				TEXT("共 %d 条悬空软引用 (未保存的包): 这是「整理后引用丢失」的主要来源, 请先修好再保存。"),
				DanglingSoftRefs.Num()));

			LastDiffText += FString::Printf(TEXT("\n\n[悬空软引用] %d 条\n"), DanglingSoftRefs.Num());
			for (const FDoodleReferenceAudit::FDiffEntry& Entry : DanglingSoftRefs)
			{
				LastDiffText += FString::Printf(TEXT("%s\n    -> %s\n"), *Entry.Package.ToString(), *Entry.Dependency.ToString());
			}
			SetDiagnosticsText(LastDiffText);

			if (ReportFile.IsEmpty())
			{
				ReportFile = FDoodleReferenceAudit::WriteReportFile(TEXT("dangling_softrefs"), LastDiffText);
			}
		}
	}

	// 5) 呈现
	LastReport = Report;
	RefreshReportBar();
	DoodleOrganize::NotifyReport(Report, ReportFile);
}

// ---------------------------------------------------------------------------
// 整理
// ---------------------------------------------------------------------------

TArray<FAssetData> SDoodleOrganizePanel::GetContentBrowserSelection() const
{
	TArray<FAssetData> Selection;
	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserModule.Get().GetSelectedAssets(Selection);
	return Selection;
}

void SDoodleOrganizePanel::SyncBrowserToAsset(const FAssetData& Asset) const
{
	if (!Asset.IsValid())
	{
		return;
	}
	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FAssetData> Assets;
	Assets.Add(Asset);
	ContentBrowserModule.Get().SyncBrowserToAssets(Assets);
}

bool SDoodleOrganizePanel::EnsureTargetFolderName(FString& OutFolderName) const
{
	OutFolderName = TargetFolderNameEdit.TrimStartAndEnd();
	if (OutFolderName.IsEmpty())
	{
		DoodleOrganize::ShowMessage(FText::FromString(TEXT("目标文件夹名称不能为空。")));
		return false;
	}
	return true;
}

void SDoodleOrganizePanel::SaveEditableSettings()
{
	UDoodleOrganizeSettings* Settings = GetMutableDefault<UDoodleOrganizeSettings>();
	if (Settings == nullptr)
	{
		return;
	}
	Settings->TargetFolderName = TargetFolderNameEdit;
	Settings->CharacterFolderName = CharacterFolderNameEdit;
	Settings->Suffix = SuffixEdit;
	Settings->SaveConfig();
}

FReply SDoodleOrganizePanel::OnOrganizeSelected()
{
	FString FolderName;
	if (!EnsureTargetFolderName(FolderName))
	{
		return FReply::Handled();
	}

	const TArray<FAssetData> Selection = GetContentBrowserSelection();
	if (Selection.Num() == 0)
	{
		DoodleOrganize::ShowMessage(FText::FromString(TEXT("请先在内容浏览器中选择要整理的资源。")));
		return FReply::Handled();
	}

	const int32 WorldCount = FDoodleAssetOrganizer::CountWorldsIn(Selection);
	if (WorldCount > 0)
	{
		const FString Message = FString::Printf(
			TEXT("选中的资源里有 %d 个地图, 会被移动到 /Game/%s/Maps/。\n\n")
			TEXT("如果是 World Partition 地图, 它的 __ExternalActors__ 目录会尝试一并搬迁; 搬不动会在报告里给出需要手工移动的路径。\n\n继续吗?"),
			WorldCount, *FolderName);

		if (DoodleOrganize::Confirm(FText::FromString(TEXT("整理选中资源")), FText::FromString(Message)) != EAppReturnType::Yes)
		{
			return FReply::Handled();
		}
	}

	RunOperation(FText::FromString(TEXT("整理选中资源")), [this, FolderName]()
	{
		FDoodleAssetOrganizer Organizer;
		return Organizer.OrganizeSelected(GetContentBrowserSelection(), FolderName);
	});

	return FReply::Handled();
}

FReply SDoodleOrganizePanel::OnOrganizeAll()
{
	const FString CharacterName = CharacterFolderNameEdit.TrimStartAndEnd();
	if (CharacterName.IsEmpty())
	{
		DoodleOrganize::ShowMessage(FText::FromString(TEXT("角色拼音名称不能为空。")));
		return FReply::Handled();
	}

	TArray<FAssetData> AllAssets;
	FDoodleAssetOrganizer::EnumerateAssets(DoodleOrganize::GetGameRootPath(), nullptr, AllAssets);
	const int32 WorldCount = FDoodleAssetOrganizer::CountWorldsIn(AllAssets);

	FString Message = FString::Printf(
		TEXT("会扫描 /Game 下的 %d 个资产, 按规则整理到 /Game/Character/%s/。\n\n")
		TEXT("这是大批量移动, 请先在工程副本上验证。\n自动快照与自动引用校验在「诊断」页开关。"),
		AllAssets.Num(), *CharacterName);

	if (WorldCount > 0)
	{
		Message += FString::Printf(
			TEXT("\n\n其中包含 %d 个地图, 会一并移动 (可在项目设置里关掉「整理地图」)。"), WorldCount);
	}

	if (DoodleOrganize::Confirm(FText::FromString(TEXT("整理所有资源")), FText::FromString(Message)) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	RunOperation(FText::FromString(TEXT("整理所有资源")), [CharacterName]()
	{
		FDoodleAssetOrganizer Organizer;
		return Organizer.OrganizeCharacterAssets(CharacterName);
	});

	return FReply::Handled();
}

// ---------------------------------------------------------------------------
// 重复贴图
// ---------------------------------------------------------------------------

TSharedPtr<FDoodleDupRowItem> SDoodleOrganizePanel::MakeDupRow(const FAssetData& Asset,
	const TSharedPtr<FDoodleDupRowItem>& Parent) const
{
	TSharedPtr<FDoodleDupRowItem> Row = MakeShareable(new FDoodleDupRowItem());
	Row->bIsGroup = false;
	Row->AssetName = Asset.AssetName;
	Row->Asset = Asset;
	Row->DisplayPath = Asset.GetObjectPathString();
	Row->Parent = Parent;
	return Row;
}

void SDoodleOrganizePanel::RebuildDupTree(const TArray<FDoodleTextureDedupService::FDupGroup>& Groups)
{
	DupRows.Reset();

	for (int32 GroupIndex = 0; GroupIndex < Groups.Num(); ++GroupIndex)
	{
		const FDoodleTextureDedupService::FDupGroup& Group = Groups[GroupIndex];

		TSharedPtr<FDoodleDupRowItem> Root = MakeShareable(new FDoodleDupRowItem());
		Root->bIsGroup = true;
		Root->AssetName = Group.AssetName;
		Root->Group = Group;
		Root->GroupIndex = GroupIndex;
		Root->DisplayPath = Group.AssetName.ToString();

		for (const FAssetData& Instance : Group.Instances)
		{
			TSharedPtr<FDoodleDupRowItem> Child = MakeDupRow(Instance, Root);
			Child->GroupIndex = GroupIndex;
			Root->Children.Add(Child);
		}

		DupRows.Add(Root);
	}

	if (DupTree.IsValid())
	{
		DupTree->RequestTreeRefresh();
	}
}

TSharedRef<ITableRow> SDoodleOrganizePanel::MakeDupRowWidget(TSharedPtr<FDoodleDupRowItem> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SDoodleDuplicateTextureRow, OwnerTable, Item)
		.ThumbnailPool(ThumbnailPool)
		.OnAssignClicked(FDoodleDupRowAction::CreateSP(this, &SDoodleOrganizePanel::OnConsolidateOne))
		.OnRenameCommitted(FDoodleRenameCommit::CreateSP(this, &SDoodleOrganizePanel::OnRenameCommitted));
}

void SDoodleOrganizePanel::HandleGetChildrenForDupTree(TSharedPtr<FDoodleDupRowItem> Item,
	TArray<TSharedPtr<FDoodleDupRowItem>>& OutChildren)
{
	OutChildren = Item.IsValid() ? Item->GetChildren() : TArray<TSharedPtr<FDoodleDupRowItem>>();
}

void SDoodleOrganizePanel::OnDupRowDoubleClick(TSharedPtr<FDoodleDupRowItem> Item)
{
	if (Item.IsValid() && !Item->bIsGroup)
	{
		SyncBrowserToAsset(Item->Asset);
	}
}

TSharedPtr<SWidget> SDoodleOrganizePanel::OnDupTreeContextMenu()
{
	if (!NowSelectedDupRow.IsValid() || NowSelectedDupRow->bIsGroup)
	{
		return SNullWidget::NullWidget;
	}

	FMenuBuilder MenuBuilder(/*bShouldCloseWindowAfterMenuSelection*/ true, nullptr);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("重命名")),
		FText::FromString(TEXT("就地修改这个资产的名字 (回车提交)")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			if (NowSelectedDupRow.IsValid())
			{
				NowSelectedDupRow->bEditing = true;
				if (DupTree.IsValid())
				{
					DupTree->RequestTreeRefresh();
				}
			}
		})));

	return MenuBuilder.MakeWidget();
}

FReply SDoodleOrganizePanel::OnFindDuplicates()
{
	FDoodleTextureDedupService Service;
	LastDupGroups = Service.FindDuplicates(DoodleOrganize::GetGameRootPath(), /*bTexturesOnly*/ true);
	RebuildDupTree(LastDupGroups);

	int32 InstanceCount = 0;
	for (const FDoodleTextureDedupService::FDupGroup& Group : LastDupGroups)
	{
		InstanceCount += Group.Instances.Num();
	}

	SetDiagnosticsText(FString::Printf(TEXT("重复贴图: %d 组, 共 %d 个副本"), LastDupGroups.Num(), InstanceCount));
	DoodleOrganize::NotifyInfo(FText::FromString(
		FString::Printf(TEXT("查找完成: %d 组重复贴图, %d 个副本"), LastDupGroups.Num(), InstanceCount)));

	return FReply::Handled();
}

FReply SDoodleOrganizePanel::OnConsolidateAllDuplicates()
{
	if (LastDupGroups.Num() == 0)
	{
		OnFindDuplicates();
	}

	if (LastDupGroups.Num() == 0)
	{
		DoodleOrganize::NotifyInfo(FText::FromString(TEXT("没有找到重复贴图")));
		return FReply::Handled();
	}

	int32 ExtraCopies = 0;
	for (const FDoodleTextureDedupService::FDupGroup& Group : LastDupGroups)
	{
		ExtraCopies += FMath::Max(0, Group.Instances.Num() - 1);
	}

	const FString Message = FString::Printf(
		TEXT("将对 %d 组重复贴图各保留一个, 把其余 %d 个副本合并过去并删除。\n\n")
		TEXT("这会改写引用并删除资产, 不可撤销。继续吗?"),
		LastDupGroups.Num(), ExtraCopies);

	if (DoodleOrganize::Confirm(FText::FromString(TEXT("一键删除重复贴图")), FText::FromString(Message)) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	RunOperation(FText::FromString(TEXT("一键删除重复贴图")), [this]()
	{
		FDoodleTextureDedupService Service;

		FDoodleOrganizeReport Report;
		Report.Operation = TEXT("ConsolidateAllDuplicates");
		Report.NumRequested = LastDupGroups.Num();

		for (const FDoodleTextureDedupService::FDupGroup& Group : LastDupGroups)
		{
			if (Group.Instances.Num() < 2)
			{
				continue;
			}

			// 保留按包名排序后的第一个 (确定性行为; 原实现保留的是数组 Top, 顺序不稳定)
			const FDoodleOrganizeReport GroupReport = Service.Consolidate(Group, Group.Instances[0]);
			Report.NumMoved += GroupReport.NumMoved;
			Report.NumFailed += GroupReport.NumFailed;
			Report.NumSkipped += GroupReport.NumSkipped;
			Report.Failures.Append(GroupReport.Failures);
			Report.Skipped.Append(GroupReport.Skipped);
			Report.Warnings.Append(GroupReport.Warnings);
		}

		Report.Finalize();
		return Report;
	});

	OnFindDuplicates();
	return FReply::Handled();
}

void SDoodleOrganizePanel::OnConsolidateOne(TSharedPtr<FDoodleDupRowItem> Item)
{
	if (!Item.IsValid() || Item->bIsGroup || !Item->Asset.IsValid())
	{
		return;
	}
	if (!LastDupGroups.IsValidIndex(Item->GroupIndex))
	{
		return;
	}

	const FDoodleTextureDedupService::FDupGroup Group = LastDupGroups[Item->GroupIndex];
	const FAssetData Keep = Item->Asset;

	const FString Message = FString::Printf(
		TEXT("保留:\n  %s\n\n把同组的其它 %d 个副本合并到它并删除。继续吗?"),
		*Keep.GetObjectPathString(), FMath::Max(0, Group.Instances.Num() - 1));

	if (DoodleOrganize::Confirm(FText::FromString(TEXT("指定保留")), FText::FromString(Message)) != EAppReturnType::Yes)
	{
		return;
	}

	RunOperation(FText::FromString(TEXT("合并重复贴图")), [Group, Keep]()
	{
		FDoodleTextureDedupService Service;
		return Service.Consolidate(Group, Keep);
	});

	OnFindDuplicates();
}

void SDoodleOrganizePanel::OnRenameCommitted(const FAssetData& Asset, const FString& Folder, const FString& NewName)
{
	RunOperation(FText::FromString(TEXT("重命名")), [Asset, Folder, NewName]()
	{
		FDoodleAssetOrganizer Organizer;
		return Organizer.RenameAssetTo(Asset, Folder, NewName);
	});

	OnFindDuplicates();
}

// ---------------------------------------------------------------------------
// 批量重命名
// ---------------------------------------------------------------------------

FReply SDoodleOrganizePanel::OnAddSuffix()
{
	const TArray<FAssetData> Selection = GetContentBrowserSelection();
	if (Selection.Num() == 0)
	{
		DoodleOrganize::ShowMessage(FText::FromString(TEXT("请先在内容浏览器中选择资源。")));
		return FReply::Handled();
	}

	const FString Suffix = SuffixEdit.TrimStartAndEnd();
	if (Suffix.IsEmpty())
	{
		DoodleOrganize::ShowMessage(FText::FromString(TEXT("请先填写后缀。")));
		return FReply::Handled();
	}

	RunOperation(FText::FromString(TEXT("添加后缀")), [Selection, Suffix]()
	{
		FDoodleAssetOrganizer Organizer;
		return Organizer.AddSuffix(Selection, Suffix);
	});

	return FReply::Handled();
}

FReply SDoodleOrganizePanel::OnRemoveSuffix()
{
	const TArray<FAssetData> Selection = GetContentBrowserSelection();
	if (Selection.Num() == 0)
	{
		DoodleOrganize::ShowMessage(FText::FromString(TEXT("请先在内容浏览器中选择资源。")));
		return FReply::Handled();
	}

	const FString Suffix = SuffixEdit.TrimStartAndEnd();
	if (Suffix.IsEmpty() && !UDoodleOrganizeSettings::Get().bLegacyRemoveSuffixBehavior)
	{
		DoodleOrganize::ShowMessage(FText::FromString(TEXT("请先填写后缀 (或在项目设置里打开「去后缀沿用旧行为」)。")));
		return FReply::Handled();
	}

	RunOperation(FText::FromString(TEXT("去除后缀")), [Selection, Suffix]()
	{
		FDoodleAssetOrganizer Organizer;
		return Organizer.RemoveSuffix(Selection, Suffix);
	});

	return FReply::Handled();
}

FReply SDoodleOrganizePanel::OnFindLongPaths()
{
	TArray<FAssetData> AllAssets;
	FDoodleAssetOrganizer::EnumerateAssets(DoodleOrganize::GetGameRootPath(), nullptr, AllAssets);

	PathRows.Reset();

	for (const FAssetData& Asset : AllAssets)
	{
		// 原实现用 GetAsset()->IsA<UWorld>() 判断, 会把每个资产都加载进来
		const UClass* AssetClass = Asset.GetClass();
		const bool bIsWorld = AssetClass != nullptr && AssetClass->IsChildOf(UWorld::StaticClass());

		const FString Filename = DoodleOrganize::GetAssetDiskFilename(Asset.PackageName.ToString(), bIsWorld);
		if (Filename.Len() < 256)
		{
			continue;
		}

		TSharedPtr<FDoodlePathRowItem> Row = MakeShareable(new FDoodlePathRowItem());
		Row->Asset = Asset;
		Row->DisplayPath = Filename;
		Row->PathLength = Filename.Len();
		PathRows.Add(Row);
	}

	PathRows.Sort([](const TSharedPtr<FDoodlePathRowItem>& A, const TSharedPtr<FDoodlePathRowItem>& B)
	{
		return A->PathLength > B->PathLength;
	});

	if (PathList.IsValid())
	{
		PathList->RequestListRefresh();
	}

	SetDiagnosticsText(FString::Printf(TEXT("路径过长 (>256) 的资源: %d 个"), PathRows.Num()));
	DoodleOrganize::NotifyInfo(FText::FromString(FString::Printf(TEXT("查找完成: %d 个过长路径"), PathRows.Num())));

	return FReply::Handled();
}

FReply SDoodleOrganizePanel::OnExportLongPaths()
{
	if (PathRows.Num() == 0)
	{
		OnFindLongPaths();
	}

	FString Csv = TEXT("AssetPath,DiskPath,Length\n");
	for (const TSharedPtr<FDoodlePathRowItem>& Row : PathRows)
	{
		if (!Row.IsValid())
		{
			continue;
		}
		Csv += FString::Printf(TEXT("\"%s\",\"%s\",%d\n"),
			*Row->Asset.GetObjectPathString(), *Row->DisplayPath, Row->PathLength);
	}

	const FString ReportFile = FDoodleReferenceAudit::WriteReportFile(TEXT("long_paths"), Csv);
	if (ReportFile.IsEmpty())
	{
		DoodleOrganize::NotifyError(FText::FromString(TEXT("导出失败")));
	}
	else
	{
		DoodleOrganize::NotifySuccess(FText::FromString(FString::Printf(TEXT("已导出: %s"), *ReportFile)));
	}

	return FReply::Handled();
}

TSharedRef<ITableRow> SDoodleOrganizePanel::MakePathRowWidget(TSharedPtr<FDoodlePathRowItem> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SDoodleAssetPathRow, OwnerTable, Item);
}

void SDoodleOrganizePanel::OnPathRowDoubleClick(TSharedPtr<FDoodlePathRowItem> Item)
{
	if (Item.IsValid())
	{
		SyncBrowserToAsset(Item->Asset);
	}
}

// ---------------------------------------------------------------------------
// 贴图工具
// ---------------------------------------------------------------------------

FReply SDoodleOrganizePanel::OnPullEngineTextures()
{
	FString FolderName;
	if (!EnsureTargetFolderName(FolderName))
	{
		return FReply::Handled();
	}

	const FString Message = FString::Printf(
		TEXT("会把 /Game 材质引用到的引擎内置贴图复制到 /Game/%s/Tex/ 下, 并把引用改指到本地副本。\n\n")
		TEXT("引擎内容里的原贴图会先备份到 Tex/Backup/。这个操作会改写引用关系, 请先在工程副本上验证。\n\n继续吗?"),
		*FolderName);

	if (DoodleOrganize::Confirm(FText::FromString(TEXT("本地化引擎贴图")), FText::FromString(Message)) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	RunOperation(FText::FromString(TEXT("本地化引擎贴图")), [FolderName]()
	{
		FDoodleAssetOrganizer Organizer;
		return Organizer.PullEngineTexturesReferencedByMaterials(FolderName);
	});

	return FReply::Handled();
}

FReply SDoodleOrganizePanel::OnResizeTextures()
{
	const FString Message = TEXT("会把 /Game 下所有尺寸不是 2 的幂的贴图重置为 2 的幂。\n\n这个操作会修改并保存贴图资产。继续吗?");

	if (DoodleOrganize::Confirm(FText::FromString(TEXT("重置贴图尺寸")), FText::FromString(Message)) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	RunOperation(FText::FromString(TEXT("重置贴图尺寸")), []()
	{
		FDoodleAssetOrganizer Organizer;
		return Organizer.ResizeTexturesToPowerOfTwo(DoodleOrganize::GetGameRootPath());
	});

	return FReply::Handled();
}

// ---------------------------------------------------------------------------
// 目录清理
// ---------------------------------------------------------------------------

FReply SDoodleOrganizePanel::OnDeleteEmptyDirectories()
{
	const FString Message = TEXT("会删除 /Game 下「没有任何文件, 且资产注册表里也没有资产」的目录。\n\n__ExternalActors__ / __ExternalObjects__ 目录不会被删。继续吗?");

	if (DoodleOrganize::Confirm(FText::FromString(TEXT("删除所有空文件夹")), FText::FromString(Message)) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	RunOperation(FText::FromString(TEXT("删除空文件夹")), []()
	{
		FDoodleAssetOrganizer Organizer;
		return Organizer.DeleteEmptyDirectories(DoodleOrganize::GetGameRootPath());
	});

	return FReply::Handled();
}

// ---------------------------------------------------------------------------
// 诊断
// ---------------------------------------------------------------------------

FReply SDoodleOrganizePanel::OnCaptureSnapshot()
{
	FDoodleReferenceAudit::FSnapshot Snapshot;
	if (!FDoodleReferenceAudit::Capture(DoodleOrganize::GetGameRootPath(), Snapshot))
	{
		DoodleOrganize::NotifyError(FText::FromString(TEXT("采集引用快照失败")));
		return FReply::Handled();
	}

	LastSnapshotFilename = FDoodleReferenceAudit::MakeSnapshotFilename(TEXT("manual"));
	if (!Snapshot.Save(LastSnapshotFilename))
	{
		LastSnapshotFilename.Reset();
		DoodleOrganize::NotifyError(FText::FromString(TEXT("写入快照失败")));
		return FReply::Handled();
	}

	SetDiagnosticsText(FString::Printf(
		TEXT("快照已保存:\n%s\n\n包: %d\n硬引用边: %d\n软引用边: %d"),
		*LastSnapshotFilename,
		Snapshot.Packages.Num(),
		Snapshot.HardEdges.Num(),
		Snapshot.SoftEdges.Num()));

	DoodleOrganize::NotifySuccess(FText::FromString(TEXT("引用快照已保存")));
	return FReply::Handled();
}

FReply SDoodleOrganizePanel::OnCompareWithLastSnapshot()
{
	if (LastSnapshotFilename.IsEmpty())
	{
		DoodleOrganize::ShowMessage(FText::FromString(
			TEXT("还没有可比较的快照。\n请先点「拍摄快照」, 或先执行一次会拍快照的操作 (整理页的操作默认会拍)。")));
		return FReply::Handled();
	}

	FDoodleReferenceAudit::FSnapshot Before;
	if (!FDoodleReferenceAudit::FSnapshot::Load(LastSnapshotFilename, Before))
	{
		DoodleOrganize::NotifyError(FText::FromString(TEXT("读取快照失败")));
		return FReply::Handled();
	}

	FDoodleReferenceAudit::FSnapshot After;
	if (!FDoodleReferenceAudit::Capture(DoodleOrganize::GetGameRootPath(), After))
	{
		DoodleOrganize::NotifyError(FText::FromString(TEXT("采集当前引用失败")));
		return FReply::Handled();
	}

	FDoodleReferenceAudit::FDiffOptions Options;
	Options.MoveMap = LastReport.MovedPackages;

	const FDoodleReferenceAudit::FDiffResult DiffResult = FDoodleReferenceAudit::Diff(Before, After, Options);
	LastDiffText = DiffResult.ToText(200);
	SetDiagnosticsText(LastDiffText);

	FDoodleReferenceAudit::WriteReportFile(TEXT("refdiff"), LastDiffText);
	FDoodleReferenceAudit::WriteReportFile(TEXT("refdiff_csv"), DiffResult.ToCsv());

	if (DiffResult.HasRealLoss())
	{
		DoodleOrganize::NotifyError(FText::FromString(FString::Printf(
			TEXT("检测到 %d 条真·引用丢失"), DiffResult.LostEdges.Num())));
	}
	else
	{
		DoodleOrganize::NotifyInfo(FText::FromString(FString::Printf(
			TEXT("没有真·引用丢失 (正确改指 %d 条, 目标消失 %d 条)"),
			DiffResult.RetargetedEdges.Num(), DiffResult.LostDependencies.Num())));
	}

	return FReply::Handled();
}

FReply SDoodleOrganizePanel::OnFindDangling()
{
	const TArray<FDoodleReferenceAudit::FDiffEntry> Dangling =
		FDoodleReferenceAudit::FindDanglingReferences(DoodleOrganize::GetGameRootPath());

	FString Text = FString::Printf(TEXT("悬空引用 %d 条 (来自资产注册表, 看不到未保存的包):\n"), Dangling.Num());
	for (int32 Index = 0; Index < Dangling.Num() && Index < 500; ++Index)
	{
		Text += FString::Printf(TEXT("  %s -> %s\n"),
			*Dangling[Index].Package.ToString(), *Dangling[Index].Dependency.ToString());
	}

	SetDiagnosticsText(Text);
	FDoodleReferenceAudit::WriteReportFile(TEXT("dangling"), Text);

	DoodleOrganize::NotifyInfo(FText::FromString(FString::Printf(TEXT("悬空引用 %d 条"), Dangling.Num())));
	return FReply::Handled();
}

FReply SDoodleOrganizePanel::OnRetargetSoftRefs()
{
	if (LastReport.MovedPackages.Num() == 0)
	{
		DoodleOrganize::NotifyWarning(FText::FromString(
			TEXT("没有可用的移动表。请先做一次整理, 或改用控制台命令:\n")
			TEXT("Doodle.Organize.RetargetSoftRefs <RootPath> <旧包> <新包> ...")));
		return FReply::Handled();
	}

	TArray<FString> TouchedPackages;
	const int32 NumRetargeted = FDoodleReferenceAudit::RetargetSoftReferences(
		LastReport.MovedPackages, DoodleOrganize::GetGameRootPath(), TouchedPackages);

	FString Text = FString::Printf(
		TEXT("补修软引用 %d 条, 涉及 %d 个包\n")
		TEXT("移动表条目: %d\n\n")
		TEXT("下面这些包已被标记为已修改, 必须保存才会落盘:\n\n"),
		NumRetargeted, TouchedPackages.Num(), LastReport.MovedPackages.Num());

	for (const FString& PackageName : TouchedPackages)
	{
		Text += FString::Printf(TEXT("%s\n"), *PackageName);
	}

	if (TouchedPackages.Num() == 0)
	{
		Text += TEXT("(没有需要补修的包 —— 说明引擎已经修好了, 或者引用者都不在内存里)\n");
	}

	SetDiagnosticsText(Text);
	const FString ReportFile = FDoodleReferenceAudit::WriteReportFile(TEXT("retarget_softrefs"), Text);

	if (NumRetargeted > 0)
	{
		DoodleOrganize::NotifySuccess(FText::FromString(FString::Printf(
			TEXT("补修了 %d 条软引用, %d 个包待保存。报告: %s"),
			NumRetargeted, TouchedPackages.Num(), *ReportFile)));
	}
	else
	{
		DoodleOrganize::NotifyInfo(FText::FromString(TEXT("没有需要补修的软引用")));
	}

	return FReply::Handled();
}

FReply SDoodleOrganizePanel::OnFindDanglingSoftRefs()
{
	// 全量扫描 (连已保存的包一起查), 这是手工诊断用的
	const TArray<FDoodleReferenceAudit::FDiffEntry> Dangling =
		FDoodleReferenceAudit::FindDanglingSoftReferences(DoodleOrganize::GetGameRootPath(),
			/*bDirtyPackagesOnly*/ false);

	FString Text = FString::Printf(
		TEXT("悬空软引用 %d 条 (扫描范围: 全部已加载对象)\n")
		TEXT("说明: 引擎的引用修复只用注册表的引用者集合; 未保存的包不在里面, 而软引用\n")
		TEXT("(TSoftObjectPtr / FSoftObjectPath) 不会跟着内存里的重命名走 —— 保存后就是死路径。\n\n"),
		Dangling.Num());

	for (int32 Index = 0; Index < Dangling.Num() && Index < 500; ++Index)
	{
		Text += FString::Printf(TEXT("%s\n    -> %s\n    %s\n"),
			*Dangling[Index].Package.ToString(),
			*Dangling[Index].Dependency.ToString(),
			*Dangling[Index].Note);
	}

	SetDiagnosticsText(Text);

	const FString ReportFile = FDoodleReferenceAudit::WriteReportFile(TEXT("dangling_softrefs"), Text);
	if (Dangling.Num() > 0)
	{
		DoodleOrganize::NotifyError(FText::FromString(FString::Printf(
			TEXT("发现 %d 条悬空软引用, 报告: %s"), Dangling.Num(), *ReportFile)));
	}
	else
	{
		DoodleOrganize::NotifySuccess(FText::FromString(TEXT("没有发现悬空软引用")));
	}

	return FReply::Handled();
}

FReply SDoodleOrganizePanel::OnOpenReportDirectory()
{
	const FString Directory = FDoodleReferenceAudit::GetReportDirectory();
	FPlatformProcess::ExploreFolder(*Directory);
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
