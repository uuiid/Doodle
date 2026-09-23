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
