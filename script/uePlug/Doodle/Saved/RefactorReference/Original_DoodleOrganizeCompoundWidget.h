// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "Stats/StatsHierarchical.h"

class UDoodleOrganizeCompoundWidget;
class FTreeItem;

DECLARE_DELEGATE_OneParam(FTreeItemParamDelegate, TSharedPtr<FTreeItem>);

class FTreeItem : public TSharedFromThis<FTreeItem>
{
public:
	FName Name{ TEXT("") };
	FString Path{TEXT("")};
	FAssetData Asset;
	TSharedPtr<FSlateBrush> Brush;
	TSharedPtr<FTreeItem> parent;
	TArray<TSharedPtr<FTreeItem>> Children;

	TArray<TSharedPtr<FTreeItem>>& GetChildren();
	FTreeItemParamDelegate OnClickedEvent;
};

class FListItem : public TSharedFromThis<FListItem>
{
public:
	FString Path{ TEXT("") };
	FAssetData Asset;
};

class STextureTreeItem : public SMultiColumnTableRow<TSharedPtr<FTreeItem>>
{
	SLATE_BEGIN_ARGS(STextureTreeItem) 
		:_OnTextCommittedEvent() {}
		SLATE_EVENT(FTreeItemParamDelegate, OnTextCommittedEvent)
	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, const TSharedPtr<FTreeItem> InTreeElement);

	// SMultiColumnTableRow overrides
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	const FSlateBrush* ShowImage() const;

	const TSharedRef<SWidget> GetWidgetFromColumnId(FName ColumnId)
	{
		return *SMultiColumnTableRow::GetWidgetFromColumnId(ColumnId);
	}
	TSharedPtr<SEditableText> EditableText;
private:
	TSharedPtr<FTreeItem> WeakTreeElement;
	FTreeItemParamDelegate OnTextCommittedEvent;
};
/**
 * 
 */
class DOODLEEDITOR_API UDoodleOrganizeCompoundWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(UDoodleOrganizeCompoundWidget)
		{}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);

	const static FName Name;
	FString TargetFolderName{""};
	
	static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

	const FString TexturePath{TEXT("Tex")};
	const FString GamePath{ TEXT("/Game/") };
private:
	FReply GenerateFolders();
	FReply GetAllRepetitiveTexture();
	FReply OneTouchDeleteRepectTexture();

	TSharedPtr<STreeView<TSharedPtr<FTreeItem>>> TreeView;
	TArray<TSharedPtr<FTreeItem>> RootChildren;

	TSharedRef<ITableRow> MakeTableRowWidget(TSharedPtr<FTreeItem> InTreeElement, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleGetChildrenForTree(TSharedPtr<FTreeItem> InItem, TArray<TSharedPtr<FTreeItem>>& OutChildren);
	void MakeDirectoryByType(FString FolderPath, FAssetData SelectedData, FString DirectoryName);

	void DeleteAllEmptyDirectory();
	FReply GetReferenceEngineTexture();
	void OnAssginRepeatTexture(TSharedPtr<FTreeItem> Item);
	FReply OnResizeTextureSize();
	void FixupAllReferencers();
	void OnRenameTexture();
	TSharedPtr<FTreeItem> NowSelectItem;
	//------------------
	FString ModeFolderName{""};
	FReply GenerateModeFolders();
	void MakeDirectoryByTypeMode(FString FolderPath, FAssetData SelectedData, FString DirectoryName);
	FString TheSuffix{ "" };
	FReply RemoveSuffix();
	FReply AddSuffix();
	TSharedPtr<SListView<TSharedPtr<FListItem>>> ListViewMode;
	TArray<TSharedPtr<FListItem>> ListItemsSourceMode;
	TSharedRef<ITableRow> MakeTableRowWidgetList(TSharedPtr<FListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);
};
