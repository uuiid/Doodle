// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Doodle/Organize/DoodleOrganizeTypes.h"

/**
 * 重复资产 (默认只扫贴图) 的查找与合并。
 * 与原实现保持同样的判定口径: 同类 + 同名 + 不同目录。
 */
class DOODLEEDITOR_API FDoodleTextureDedupService
{
public:
	struct FDupGroup
	{
		FName AssetName;
		FTopLevelAssetPath ClassPath;
		TArray<FAssetData> Instances;
	};

	/** 找出 RootPath 下「同类同名但不同目录」的资产 */
	TArray<FDupGroup> FindDuplicates(const FString& RootPath, bool bTexturesOnly = true) const;

	/**
	 * 把 Group 中除 Keep 之外的实例合并到 Keep。
	 * 语义与原来的「指定」按钮一致: 保留点击的那一个, 其余合并过来并被删除。
	 */
	FDoodleOrganizeReport Consolidate(const FDupGroup& Group, const FAssetData& Keep) const;
};
