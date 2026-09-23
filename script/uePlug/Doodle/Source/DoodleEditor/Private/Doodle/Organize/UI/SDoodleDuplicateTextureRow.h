// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Doodle/Organize/UI/DoodleOrganizeRowItems.h"
#include "Widgets/Views/STableRow.h"

class FAssetThumbnail;
class FAssetThumbnailPool;

/**
 * 重复贴图树的行控件。
 * 只负责显示与抛委托, 不做任何资产操作 (原实现直接在行里调 RenameAsset)。
 */
class SDoodleDuplicateTextureRow : public SMultiColumnTableRow<TSharedPtr<FDoodleDupRowItem>>
{
public:
	SLATE_BEGIN_ARGS(SDoodleDuplicateTextureRow)
		: _ThumbnailPool(nullptr)
	{}
		SLATE_ARGUMENT(TSharedPtr<FAssetThumbnailPool>, ThumbnailPool)
		SLATE_EVENT(FDoodleDupRowAction, OnAssignClicked)
		SLATE_EVENT(FDoodleRenameCommit, OnRenameCommitted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable,
		const TSharedPtr<FDoodleDupRowItem> InItem);

	//~ SMultiColumnTableRow
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	FText GetPathText() const;
	FText GetGroupText() const;
	bool IsEditable() const;
	void OnPathCommitted(const FText& InText, ETextCommit::Type InCommitType);
	FReply OnAssignClicked();
	EVisibility GetAssignVisibility() const;

	TSharedPtr<FDoodleDupRowItem> Item;
	TSharedPtr<FAssetThumbnail> Thumbnail;

	FDoodleDupRowAction OnAssignClickedDelegate;
	FDoodleRenameCommit OnRenameCommittedDelegate;
};
