// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "DoodleOrganizeSettings.generated.h"

/**
 * Doodle 文件整理的持久化设置。
 * 取代原实现在 GEngineIni 的 [DoodleOrganize] 段里手写的 GConfig 读写
 * (原实现把 GConfig->GetString 放在 Slate 的 Text_Lambda 里, 每帧重绘都会读 ini)。
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Doodle 文件整理"))
class DOODLEEDITOR_API UDoodleOrganizeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDoodleOrganizeSettings();

#if WITH_EDITOR
	//~ UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;
#endif

	/** 「整理选中资源」的目标文件夹名 (相对 /Game) */
	UPROPERTY(config, EditAnywhere, Category = "整理", meta = (DisplayName = "目标文件夹名"))
	FString TargetFolderName;

	/** 「整理所有资源」的角色拼音名 (原 ModeFolderName) */
	UPROPERTY(config, EditAnywhere, Category = "整理", meta = (DisplayName = "角色名称"))
	FString CharacterFolderName;

	/** 批量加/去后缀使用的后缀, 不带前导下划线 (填 low -> 名字变成 T_rock_low) */
	UPROPERTY(config, EditAnywhere, Category = "整理", meta = (DisplayName = "后缀"))
	FString Suffix;

	/** 是否整理地图 (UWorld -> Maps)。默认保留原行为: 开 */
	UPROPERTY(config, EditAnywhere, Category = "整理|规则", meta = (DisplayName = "整理地图 (UWorld -> Maps)"))
	bool bIncludeWorlds = true;

	/** 未命中规则的类型是否归入 Other。默认保留原行为: 开 */
	UPROPERTY(config, EditAnywhere, Category = "整理|规则", meta = (DisplayName = "未命中类型归入 Other"))
	bool bIncludeOtherTypes = true;

	/** 跳过 World Partition 内部目录 (__ExternalActors__ / __ExternalObjects__) */
	UPROPERTY(config, EditAnywhere, Category = "整理|规则", meta = (DisplayName = "跳过 World Partition 内部目录"))
	bool bSkipInternalFolders = true;

	/** 移动地图时一并搬迁其 __ExternalActors__ 目录 (否则地图的 actor 会失联) */
	UPROPERTY(config, EditAnywhere, Category = "整理|规则", meta = (DisplayName = "移动地图时一并搬迁 __ExternalActors__ 目录"))
	bool bMoveWorldExternalActors = true;

	/** 去后缀沿用旧行为 (按最后一个下划线盲截, 会把 T_rock 变成 T) */
	UPROPERTY(config, EditAnywhere, Category = "整理|规则", meta = (DisplayName = "去后缀沿用旧行为 (按最后一个下划线盲截)"))
	bool bLegacyRemoveSuffixBehavior = false;

	/** 额外排除的类路径, 例如 /Script/LevelSequence.LevelSequence */
	UPROPERTY(config, EditAnywhere, Category = "整理|规则", meta = (DisplayName = "额外排除的类路径"))
	TArray<FString> ExtraExcludedClasses;

	/** 整理前自动拍摄引用快照 */
	UPROPERTY(config, EditAnywhere, Category = "整理|诊断", meta = (DisplayName = "整理前自动拍摄引用快照"))
	bool bAutoSnapshotBeforeOrganize = true;

	/** 整理后自动校验引用差异 */
	UPROPERTY(config, EditAnywhere, Category = "整理|诊断", meta = (DisplayName = "整理后自动校验引用差异"))
	bool bAutoVerifyAfterOrganize = true;

	/**
	 * 整理后自动扫描「未保存包里的悬空软引用」。
	 * 这是注册表快照比对看不到的盲区: 引擎的引用修复只用注册表的引用者集合,
	 * 未保存(脏)的包不在里面, 而软引用 (TSoftObjectPtr / FSoftObjectPath) 不会
	 * 跟着内存里的重命名走 —— 保存后就成了死路径。默认开。
	 */
	UPROPERTY(config, EditAnywhere, Category = "整理|诊断", meta = (DisplayName = "整理后自动扫描未保存包里的悬空软引用"))
	bool bAutoScanDanglingSoftRefs = true;

	/**
	 * 【根因修复】移动资产后, 把引擎漏掉的软引用直接改写成新路径。
	 *
	 * 引擎的 FAssetRenameManager 只用资产注册表的引用者集合来修引用, 未保存(脏)的包
	 * 不在里面; 它确实有一段脏包兜底, 但只在 bOnlyFixSoftReferences == true 时生效,
	 * 普通移动走的是 false。本地资产移动时引擎又不会留重定向器, 于是旧路径直接消失,
	 * 脏包里的 TSoftObjectPtr / FSoftObjectPath 保存后就成了死路径 —— 这就是
	 * 「整理文件后引用丢失」的根因。默认开。
	 *
	 * 被改写的包会被标脏, 需要在报告里提示用户保存。
	 */
	UPROPERTY(config, EditAnywhere, Category = "整理|诊断", meta = (DisplayName = "移动后补修引擎漏掉的软引用 (根因修复)"))
	bool bRetargetSoftReferencesAfterMove = true;

	/** 报告中最多列出的引用边数 */
	UPROPERTY(config, EditAnywhere, Category = "整理|诊断", meta = (DisplayName = "报告中最多列出的引用边数"))
	int32 MaxReportedEdges = 500;

	/**
	 * 【地图引用者】移动后追加一趟「只修引用、不重命名」的引擎修复。
	 *
	 * 引擎的 FAssetRenameManager::LoadReferencingPackages 有一个 bLoadAllPackages 开关
	 * (AssetRenameManager.cpp:921), 它决定「地图里的引用者」要不要真的加载:
	 *   - false: 地图引用者不加载, 改成给旧路径留一个重定向器 (1028 行);
	 *   - true : 全部加载并修好, 不留重定向器。
	 * 这个开关没法直接设, 它被写死成 bSoftReferencesOnly (482 行), 而
	 * bSoftReferencesOnly 只有在**所有**待重命名条目的 bOnlyFixSoftReferences 都为 true 时
	 * 才成立 (390/431 行)。而 bOnlyFixSoftReferences == true 时引擎**不做重命名**
	 * (1766 行), 只修引用 —— 正好就是我们要的第二趟。
	 *
	 * 所以这里在移动完成后, 用 (旧包 -> 新包) + bOnlyFixSoftReferences=true 再调一次
	 * RenameAssets, 让引擎把所有软引用者 (含地图、含未保存的脏包) 都加载并修好。
	 * 代价是这一趟会加载较多包。默认开。
	 */
	UPROPERTY(config, EditAnywhere, Category = "整理|诊断", meta = (DisplayName = "移动后加载所有软引用者修复地图引用 (bLoadAllPackages)"))
	bool bFixMapReferencersWithLoadAllPackages = true;

	/**
	 * 整理后清除重定向器 (旧路径不再保留引用兜底)。
	 *
	 * 安全前提: 引擎的 FixupReferencers 只会删除「没有任何失败/被锁/保存失败引用者」的
	 * 重定向器 (AssetFixUpRedirectors.cpp:948-950), 所以删除是安全的 —— 修不好的那些会留着。
	 * 另外它仍会弹一次 "Redirector Update Report" 对话框。
	 */
	UPROPERTY(config, EditAnywhere, Category = "整理|清理", meta = (DisplayName = "整理后清除重定向器"))
	bool bDeleteRedirectorsAfterOrganize = true;

	/**
	 * 整理前检测到未保存(脏)的包时自动保存它们。
	 *
	 * 为什么有用: 引擎的引用修复只用资产注册表的引用者集合, 而脏包的依赖关系还没进注册表;
	 * 先保存一次, 这些包就进了注册表, 引擎自己那一趟就能把它们修好 —— 和
	 * bRetargetSoftReferencesAfterMove 形成双保险。
	 */
	UPROPERTY(config, EditAnywhere, Category = "整理|安全", meta = (DisplayName = "整理前自动保存未保存的包 (脏包兜底)"))
	bool bSaveDirtyPackagesBeforeOrganize = true;

	/**
	 * 自动保存脏包之前先弹窗确认 (列出会被保存的包)。
	 * 关掉 = 静默保存 (仍然只在 bSaveDirtyPackagesBeforeOrganize 打开时生效)。
	 * 注意: 没有 UI 的调用方 (命令行/脚本) 在需要确认时**不会**擅自保存。
	 */
	UPROPERTY(config, EditAnywhere, Category = "整理|安全", meta = (DisplayName = "自动保存脏包前先确认"))
	bool bAskBeforeSavingDirtyPackages = true;

	/** 访问入口 (首次访问时做一次旧 ini 迁移) */
	static const UDoodleOrganizeSettings& Get();

	/** /Game/<TargetFolderName>; 未配置时返回空串 */
	FString GetTargetFolderPackagePath() const;

	/** /Game/Character/<CharacterFolderName>; 未配置时返回空串 */
	FString GetCharacterFolderPackagePath() const;

private:
	/** 从 GEngineIni 的 [DoodleOrganize] 迁移旧键 (只读不删) */
	void MigrateLegacyEngineIni(bool bAllowSave);
};
