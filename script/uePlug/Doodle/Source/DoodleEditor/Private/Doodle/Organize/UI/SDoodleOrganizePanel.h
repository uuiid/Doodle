// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Doodle/Organize/DoodleOrganizeTypes.h"
#include "Doodle/Organize/DoodleTextureDedup.h"
#include "Doodle/Organize/UI/DoodleOrganizeRowItems.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STreeView.h"

class FAssetThumbnailPool;
class STextBlock;
class SWidgetSwitcher;

/**
 * Doodle 文件整理面板。
 *
 * 责任边界:
 *   - 只做布局、命令接线、结果呈现;
 *   - 所有资产操作都交给 FDoodleAssetOrganizer / FDoodleTextureDedupService / FDoodleReferenceAudit;
 *   - 所有写操作统一走 RunOperation(): 事务 + 可选快照 + 可选引用校验 + 报告栏 + 通知。
 */
class SDoodleOrganizePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDoodleOrganizePanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	enum class ETab : uint8
	{
		Organize,
		Dedupe,
		Rename,
		Texture,
		Cleanup,
		Diagnostics,
		Count,
	};

	// ---- 布局 ----
	TSharedRef<SWidget> BuildTabBar();
	TSharedRef<SWidget> BuildOrganizeTab();
	TSharedRef<SWidget> BuildDedupeTab();
	TSharedRef<SWidget> BuildRenameTab();
	TSharedRef<SWidget> BuildTextureTab();
	TSharedRef<SWidget> BuildCleanupTab();
	TSharedRef<SWidget> BuildDiagnosticsTab();
	TSharedRef<SWidget> BuildReportBar();
	TSharedRef<SWidget> BuildSectionHeader(const FText& Text);
	TSharedRef<SWidget> BuildLabeledTextBox(const FText& Label, FString& InOutValue, const FText& Tooltip);

	void SetActiveTab(ETab InTab);
	ECheckBoxState GetTabCheckState(ETab InTab) const;
	void OnTabCheckStateChanged(ECheckBoxState NewState, ETab InTab);

	// ---- 统一操作入口 ----
	void RunOperation(const FText& OperationName, TFunctionRef<FDoodleOrganizeReport()> Operation);

	// ---- 命令: 整理 ----
	FReply OnOrganizeSelected();
	FReply OnOrganizeAll();

	// ---- 命令: 重复贴图 ----
	FReply OnFindDuplicates();
	FReply OnConsolidateAllDuplicates();
	void OnConsolidateOne(TSharedPtr<FDoodleDupRowItem> Item);
	void OnRenameCommitted(const FAssetData& Asset, const FString& Folder, const FString& NewName);

	// ---- 命令: 批量重命名 ----
	FReply OnAddSuffix();
	FReply OnRemoveSuffix();
	FReply OnFindLongPaths();
	FReply OnExportLongPaths();

	// ---- 命令: 贴图工具 ----
	FReply OnPullEngineTextures();
	FReply OnResizeTextures();

	// ---- 命令: 目录清理 ----
	FReply OnDeleteEmptyDirectories();

	// ---- 命令: 诊断 ----
	FReply OnCaptureSnapshot();
	FReply OnCompareWithLastSnapshot();
	FReply OnFindDangling();
	FReply OnFindDanglingSoftRefs();
	FReply OnRetargetSoftRefs();
	FReply OnOpenReportDirectory();

	// ---- 视图回调 ----
	TSharedRef<ITableRow> MakeDupRowWidget(TSharedPtr<FDoodleDupRowItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleGetChildrenForDupTree(TSharedPtr<FDoodleDupRowItem> Item, TArray<TSharedPtr<FDoodleDupRowItem>>& OutChildren);
	void OnDupRowDoubleClick(TSharedPtr<FDoodleDupRowItem> Item);
	TSharedPtr<SWidget> OnDupTreeContextMenu();
	TSharedRef<ITableRow> MakePathRowWidget(TSharedPtr<FDoodlePathRowItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnPathRowDoubleClick(TSharedPtr<FDoodlePathRowItem> Item);

	// ---- 辅助 ----
	void RefreshReportBar();
	void RebuildDupTree(const TArray<FDoodleTextureDedupService::FDupGroup>& Groups);
	TArray<FAssetData> GetContentBrowserSelection() const;
	void SaveEditableSettings();
	TSharedPtr<FDoodleDupRowItem> MakeDupRow(const FAssetData& Asset, const TSharedPtr<FDoodleDupRowItem>& Parent) const;
	void SetDiagnosticsText(const FString& Text);
	void SyncBrowserToAsset(const FAssetData& Asset) const;
	bool EnsureTargetFolderName(FString& OutFolderName) const;

	// ---- 状态 ----
	ETab ActiveTab = ETab::Organize;

	TSharedPtr<SWidgetSwitcher> TabSwitcher;

	TSharedPtr<STreeView<TSharedPtr<FDoodleDupRowItem>>> DupTree;
	TArray<TSharedPtr<FDoodleDupRowItem>> DupRows;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TArray<FDoodleTextureDedupService::FDupGroup> LastDupGroups;
	TSharedPtr<FDoodleDupRowItem> NowSelectedDupRow;

	TSharedPtr<SListView<TSharedPtr<FDoodlePathRowItem>>> PathList;
	TArray<TSharedPtr<FDoodlePathRowItem>> PathRows;

	TSharedPtr<STextBlock> ReportBarText;
	TSharedPtr<STextBlock> DiagnosticsText;

	FString TargetFolderNameEdit;
	FString CharacterFolderNameEdit;
	FString SuffixEdit;

	FDoodleOrganizeReport LastReport;
	FString LastDiffText;
	FString LastSnapshotFilename;
};
