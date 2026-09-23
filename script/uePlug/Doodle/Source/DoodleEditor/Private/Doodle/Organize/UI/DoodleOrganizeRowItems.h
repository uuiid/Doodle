// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Doodle/Organize/DoodleTextureDedup.h"

struct FDoodleDupRowItem;
struct FDoodlePathRowItem;

DECLARE_DELEGATE_OneParam(FDoodleDupRowAction, TSharedPtr<FDoodleDupRowItem>);

/** 行内改名提交 (资产, 目标目录, 新名字) */
DECLARE_DELEGATE_ThreeParams(FDoodleRenameCommit, const FAssetData&, const FString&, const FString&);

/**
 * 重复贴图树的行数据。
 * 编辑态 (bEditing) 归行自己管 —— 原实现是外部通过 TreeView->WidgetFromItem
 * 拿到 STextureTreeItem 再戳它的 EditableText, UI 内部状态外泄。
 */
struct FDoodleDupRowItem : public TSharedFromThis<FDoodleDupRowItem>
{
	bool bIsGroup = false;

	/** 组名 (组行) 或资产名 (叶子行) */
	FName AssetName;

	/** 组行有效 */
	FDoodleTextureDedupService::FDupGroup Group;

	/** 叶子行有效: 所属组在面板 LastDupGroups 里的下标 */
	int32 GroupIndex = INDEX_NONE;

	/** 叶子行有效 */
	FAssetData Asset;

	/** 叶子行的显示路径 */
	FString DisplayPath;

	/** 是否处于可编辑状态 */
	bool bEditing = false;

	TWeakPtr<FDoodleDupRowItem> Parent;
	TArray<TSharedPtr<FDoodleDupRowItem>> Children;

	TArray<TSharedPtr<FDoodleDupRowItem>>& GetChildren() { return Children; }
};

/** 路径列表的行数据 */
struct FDoodlePathRowItem : public TSharedFromThis<FDoodlePathRowItem>
{
	FAssetData Asset;
	FString DisplayPath;
	int32 PathLength = 0;
};
