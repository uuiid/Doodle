// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Doodle/Organize/DoodleOrganizeTypes.h"

class UPackage;

/**
 * 脏包兜底。
 *
 * 背景: 引擎的引用修复 (FAssetRenameManager::PopulateAssetReferencers) 只用资产注册表的
 * 引用者集合, 而**未保存的包**其依赖关系还没写进注册表, 所以引擎看不到它们。
 * 整理前先把这些包保存一次, 它们就进了注册表, 引擎自己那一趟就能把它们的引用一起修好 ——
 * 和 DoodleReferenceAudit::RetargetSoftReferences (内存里直接补修) 形成双保险。
 *
 * 这一层不弹窗: 需要确认时由调用方传入回调 (UI 层注入), 没有回调就不擅自保存用户的未保存工作。
 */
namespace DoodleOrganize
{
	/** 收集 RootPath 下未保存的包 (世界包 + 内容包), 已剔除编译期 / PIE 包 */
	DOODLEEDITOR_API TArray<UPackage*> FindDirtyProjectPackages(const FString& RootPath);

	/** 把包列表转成排序去重后的包名 (给报告 / 确认框显示) */
	DOODLEEDITOR_API TArray<FString> GetPackageNames(const TArray<UPackage*>& Packages);

	/** 保存这些包 (bOnlyDirty=true)。返回是否全部成功 */
	DOODLEEDITOR_API bool SavePackages(const TArray<UPackage*>& Packages);

	/**
	 * 整理前的脏包兜底。
	 *
	 * @param RootPath      只处理这个根路径下的脏包 (通常是 /Game)
	 * @param bAsk          是否需要先确认
	 * @param AskFn         确认回调 (需要 bAsk 时调用); 返回 false 表示用户拒绝
	 * @param bSave         是否真的保存 (false = 只检测并报告, 用于"预览"场景)
	 *
	 * 报告里: NumDirtyPackagesSaved / SavedDirtyPackages 有值;
	 *         用户拒绝时 Result = Cancelled, 调用方应中止整理。
	 */
	DOODLEEDITOR_API FDoodleOrganizeReport SaveDirtyPackagesBeforeOrganize(
		const FString& RootPath,
		bool bAsk,
		TFunctionRef<bool(const TArray<FString>&)> AskFn,
		bool bSave = true);
}
