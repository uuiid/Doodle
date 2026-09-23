// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

class UClass;

/**
 * 资产类型 -> 目标子目录 的规则表 (纯数据 + 纯函数)。
 * 与原实现的映射保持逐条一致, 只有显式标注的修正 (材质子类改 IsChildOf) 例外。
 */
namespace DoodleOrganize
{
	struct FTypeRule
	{
		const UClass* Class = nullptr;
		/** true = 精确类匹配; false = IsChildOf (含子类) */
		bool bExactClass = false;
		/** 相对目标根的子目录; nullptr 表示不处理 */
		const TCHAR* SubFolder = nullptr;
	};

	struct FRuleSet
	{
		FString Name;
		TArray<FTypeRule> Rules;
		/** 强制排除的类路径字符串, 例如 /Script/CoreUObject.ObjectRedirector */
		TArray<FString> AlwaysExcludedClasses;
		/** 用户在设置里额外排除的类路径 */
		TArray<FString> ExtraExcludedClasses;
		bool bIncludeOtherTypes = true;
		bool bIncludeWorlds = true;
	};

	/**
	 * 命中返回 true 并给出 OutSubFolder;
	 * 被排除 / 不处理返回 false 并给出 OutSkipReason (可直接写进报告)。
	 */
	DOODLEEDITOR_API bool TryResolveSubFolder(const FAssetData& Asset, const FRuleSet& Rules,
		FString& OutSubFolder, FString& OutSkipReason);

	/** 「整理选中资源」规则 (原 GenerateFolders) */
	DOODLEEDITOR_API const FRuleSet& GetAssetRules();

	/** 「整理所有资源」规则 (原 GenerateModeFolders) */
	DOODLEEDITOR_API const FRuleSet& GetCharacterRules();

	/** UI 上「推荐排除项」按钮使用的候选类路径 (默认不生效, 由用户自行加入设置) */
	DOODLEEDITOR_API TArray<FString> GetRecommendedExcludedClassPaths();
}
