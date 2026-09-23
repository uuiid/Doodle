// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

/**
 * 纯函数路径/命名工具。
 * 这一层不依赖 Slate, 不弹窗, 只做判断与字符串计算, 因此全部可被 automation test 覆盖。
 */
namespace DoodleOrganize
{
	/** 去掉对象路径/子对象路径, 只保留长包名。"/Game/A/B.B:Sub" -> "/Game/A/B" */
	DOODLEEDITOR_API FString StripObjectPath(const FString& PackageOrObjectPath);

	/** 拼接长包路径。CombinePackagePath("/Game/A/", "B") -> "/Game/A/B" */
	DOODLEEDITOR_API FString CombinePackagePath(const FString& InFolder, const FString& InName);

	/**
	 * 若 InPath 的包名部分命中 MoveMap (旧包名 -> 新包名), 返回改写后的完整路径 (保留对象/子对象后缀);
	 * 没命中则返回空串。
	 *
	 * 这是「整理后引用丢失」修复的核心: 软引用 (TSoftObjectPtr / FSoftObjectPath) 存的是字符串路径,
	 * 引擎的自动修复只覆盖资产注册表知道的引用者, 未保存的包会被漏掉, 需要我们自己改写。
	 *
	 * RetargetObjectPath("/Game/A/T_rock.T_rock", {"/Game/A/T_rock" -> "/Game/Tex/T_rock"})
	 *   -> "/Game/Tex/T_rock.T_rock"
	 */
	DOODLEEDITOR_API FString RetargetObjectPath(const FString& InPath, const TMap<FString, FString>& MoveMap);

	/** 是否为重定向器 (引用兜底对象), 这类资产永远不参与整理 */
	DOODLEEDITOR_API bool IsRedirector(const FAssetData& Asset);

	/** PackageName 是否位于 PackagePath 之下 (含自身) */
	DOODLEEDITOR_API bool IsUnderPackagePath(const FString& PackageName, const FString& PackagePath);

	/** 是否为 World Partition 的内部包目录 (__ExternalActors__ / __ExternalObjects__) */
	DOODLEEDITOR_API bool IsInternalPackagePath(const FString& PackageName);

	/**
	 * 是否为「不是资产」的包路径: /Script/... (C++ 原生类), /Temp/, /Transient/ 等。
	 *
	 * 这类"依赖"永远不在资产注册表里, 磁盘上也没有 .uasset, 所以如果不排除,
	 * 悬空引用扫描会把每一条指向原生类的引用都误报成丢失 —— 实测一个 224 个包的工程
	 * 会报出 140 条全是 /Script/InterchangeEngine 之类的假阳性。
	 */
	DOODLEEDITOR_API bool IsNonAssetPackagePath(const FString& PackageName);

	/** 磁盘上是否存在该包 (与 EditorAssetSubsystem::RenameAsset 的判据一致) */
	DOODLEEDITOR_API bool DoesAssetExistOnDisk(const FString& PackageOrObjectPath);

	/** 资产注册表中是否存在该包 (与 EditorAssetSubsystem::DoesAssetExist 的判据一致) */
	DOODLEEDITOR_API bool DoesAssetExistInRegistry(const FString& PackageOrObjectPath, bool bIncludeRedirectors = true);

	/** 磁盘与注册表都不存在 */
	DOODLEEDITOR_API bool IsAssetPathFree(const FString& PackageOrObjectPath);

	/**
	 * 批量场景下的空闲判定: 除了磁盘/注册表, 还要看这一批里有没有别的资产已经预订了同一个目标。
	 *
	 * 没有这一步时, 两个同名资产 (例如 /Game/CZ721/Meshs/10/ysMSK_fg01 与
	 * /Game/Character/test_1/Texture/ysMSK_fg01) 会被分配到同一个目标包 ——
	 * 注册表和磁盘当时都还没变, 只有批次内预订能发现这种冲突。引擎随后会让这一个失败,
	 * 并且因为 FixReferencesAndRename 是"有任一失败就整批返回 Failure", 还会连带
	 * 让上层误判整批都失败。
	 */
	DOODLEEDITOR_API bool IsAssetPathFreeForBatch(const FString& PackageOrObjectPath, const TSet<FString>* ReservedPackages);

	/**
	 * 求一个磁盘+注册表都不冲突的 (目录, 名字)。
	 * 若 DesiredFolder/DesiredName 本身可用则原样返回; 否则用引擎的 CreateUniqueAssetName,
	 * 并额外校验磁盘 (注册表看不见的"幽灵"文件会走手工递增分支)。
	 *
	 * @param ReservedPackages 本批次已分配出去的目标包名; 传 nullptr 表示只查磁盘/注册表
	 */
	DOODLEEDITOR_API bool TryMakeUniqueAssetPath(const FString& DesiredFolder, const FString& DesiredName,
		FString& OutFolder, FString& OutName, const TSet<FString>* ReservedPackages = nullptr);

	/** 文件系统路径 -> 长包路径 */
	DOODLEEDITOR_API bool TryConvertFilenameToPackagePath(const FString& InFilename, FString& OutPackagePath);

	/** 资产的磁盘绝对路径 (优先真实路径, 找不到则按约定推导) */
	DOODLEEDITOR_API FString GetAssetDiskFilename(const FString& PackageName, bool bIsWorld = false);

	/** "/Game" */
	DOODLEEDITOR_API FString GetGameRootPath();

	// ---------------------------------------------------------------------
	// 后缀命名 (纯函数)。Suffix 存的是不带前导下划线的串, 例如 "low" -> 名字 "T_rock_low"。
	// ---------------------------------------------------------------------

	/** 幂等: 已经以 "_<Suffix>" 结尾时返回 false (不做任何改动) */
	DOODLEEDITOR_API bool MakeSuffixedName(const FString& InName, const FString& Suffix, FString& OutName);

	/** 只在确实以 "_<Suffix>" 结尾时剥离; 剥离后为空则返回 false */
	DOODLEEDITOR_API bool MakeUnsuffixedName(const FString& InName, const FString& Suffix, FString& OutName);

	/** 旧行为 (逃生开关): 按最后一个 '_' 盲截, 不做后缀匹配 */
	DOODLEEDITOR_API bool MakeUnsuffixedNameLegacy(const FString& InName, FString& OutName);
}
