// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Doodle/Organize/DoodleOrganizeTypes.h"

class UClass;

namespace DoodleOrganize
{
	struct FRuleSet;
}

/**
 * 由 UI 层注入的「要不要保存这些脏包」确认回调。
 * 服务层不弹窗: 需要问用户时通过这个委托把问题交给 UI; 没有绑定就不擅自保存。
 */
DECLARE_DELEGATE_RetVal_OneParam(bool, FDoodleConfirmSaveDirtyPackages, const TArray<FString>& /*DirtyPackageNames*/);

/**
 * 资产整理服务层。所有资产写操作都集中在这里。
 * 这一层不出现任何 Slate 类型, 也不弹窗; 失败通过 FDoodleOrganizeReport 返回。
 */
class DOODLEEDITOR_API FDoodleAssetOrganizer
{
public:
	FDoodleAssetOrganizer();

	/** 注入确认回调 (UI 层在 Construct 里设置一次) */
	void SetDirtyPackageConfirmDelegate(FDoodleConfirmSaveDirtyPackages InDelegate)
	{
		DirtyPackageConfirm = MoveTemp(InDelegate);
	}

	/**
	 * 一次调用搬一批。
	 * 关键点: 只向引擎提交「一次」重命名请求 —— UE 的引用修复是按批次计算
	 * RenamedReferencingPackageNames 的, 逐个搬会放大失败面。
	 * bUseDialog = true 走 RenameAssetsWithDialog (与内容浏览器一致, 会弹 "About to load N assets"),
	 * bUseDialog = false 走 RenameAssets (批量整理用, 避免上千个慢任务对话框)。
	 */
	FDoodleOrganizeReport MoveAssets(TArrayView<const FDoodleMoveRequest> Requests, bool bUseDialog);

	/** 单条移动 (便于测试与行内改名) */
	FDoodleMoveOutcome MoveOneAsset(const FDoodleMoveRequest& Request, bool bUseDialog);

	/** 「整理选中资源」: 按 GetAssetRules() 分类到 /Game/<TargetFolderName>/<子目录> */
	FDoodleOrganizeReport OrganizeSelected(const TArray<FAssetData>& Assets, const FString& TargetFolderName);

	/** 「整理所有资源」: 按 GetCharacterRules() 分类到 /Game/Character/<CharacterFolderName>/<子目录> */
	FDoodleOrganizeReport OrganizeCharacterAssets(const FString& CharacterFolderName);

	/** 批量加后缀 (幂等: 已经带后缀的会跳过) */
	FDoodleOrganizeReport AddSuffix(const TArray<FAssetData>& Assets, const FString& Suffix);

	/** 批量去后缀 (默认只剥离真正匹配的后缀; 可在设置里切回旧的盲截行为) */
	FDoodleOrganizeReport RemoveSuffix(const TArray<FAssetData>& Assets, const FString& Suffix);

	/** 行内改名 (取代行控件直接调 EditorAssetSubsystem::RenameAsset) */
	FDoodleOrganizeReport RenameAssetTo(const FAssetData& Asset, const FString& DesiredFolder, const FString& DesiredName);

	/** 把 RootPath 下所有贴图重置为 2 的幂次方尺寸 */
	FDoodleOrganizeReport ResizeTexturesToPowerOfTwo(const FString& RootPath);

	/** 把 /Game 材质引用的引擎内置贴图复制到本地目录并改指 */
	FDoodleOrganizeReport PullEngineTexturesReferencedByMaterials(const FString& TargetFolderName);

	/** 删除 RootPath 下的空目录 (用包路径查注册表, 不再把文件系统路径塞进 GetAssetsByPath) */
	FDoodleOrganizeReport DeleteEmptyDirectories(const FString& RootPath);

	/** 枚举 RootPath 下的资产 (排除重定向器与 World Partition 内部目录) */
	static void EnumerateAssets(const FString& RootPath, const UClass* RequiredBaseClass, TArray<FAssetData>& OutAssets);

	/** 一批资产里会被搬动的地图数量 (供 UI 弹确认框) */
	static int32 CountWorldsIn(const TArray<FAssetData>& Assets);

	/** 目标目录是否可写; 不可写时 OutReason 给出原因 */
	static bool CheckWritable(const FString& Folder, FString& OutReason);

private:
	FDoodleOrganizeReport MoveAssetsInternal(TArrayView<const FDoodleMoveRequest> Requests, bool bUseDialog,
		TArray<FDoodleMoveOutcome>* OutOutcomes);

	void BuildOrganizeRequests(const TArray<FAssetData>& Assets, const FString& TargetRootFolder,
		const DoodleOrganize::FRuleSet& Rules, TArray<FDoodleMoveRequest>& OutRequests,
		FDoodleOrganizeReport& Report) const;

	/** 地图被搬动后, 处理它的 __ExternalActors__ 目录 (否则 WP 的 actor 会失联) */
	void HandleWorldExternalActors(FDoodleOrganizeReport& Report) const;

	/** 把旧路径上确实存在的重定向器修一遍 (按设置决定是否删除) */
	FDoodleOrganizeReport FixupRedirectorsForMovedPackages(const TMap<FName, FName>& MovedPackages) const;

	/**
	 * 第二趟: 只修引用、不重命名。
	 *
	 * 用 (旧包 -> 新包) + bOnlyFixSoftReferences = true 再调一次 RenameAssets, 这样
	 * bSoftReferencesOnly 成立 => 引擎的 bLoadAllPackages 变成 true => 地图里的引用者
	 * 也会被加载并修好 (而不是留一个重定向器), 未保存的脏包也会被一起修
	 * (AssetRenameManager.cpp:482/843/858-868/1028/1766)。
	 */
	FDoodleOrganizeReport FixReferencesOnlyForMovedPackages(const TMap<FName, FName>& MovedPackages) const;

	/** 整理前把未保存的脏包保存掉 (让依赖进注册表, 引擎那趟才能修) */
	FDoodleOrganizeReport SaveDirtyPackagesGuard() const;

	/** UI 注入的确认回调 */
	FDoodleConfirmSaveDirtyPackages DirtyPackageConfirm;
};
