// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 引用审计: 采集 /Game 下的引用边 -> 保存快照 -> 与另一次快照比对。
 *
 * 这是定位「整理文件后引用丢失」的核心工具。判定规则:
 *   - 边 (P -> D) 在整理前存在, 整理后消失:
 *       * D 被本次操作搬到了 D', 且整理后存在边 (P -> D')  -> RetargetedEdges (正常)
 *       * D 被本次操作搬到了 D', 但整理后没有 (P -> D')     -> LostEdges  (真·引用丢失)
 *       * D 未被本次操作搬动, 且 D 仍然存在                  -> LostEdges  (真·引用丢失)
 *       * D 未被本次操作搬动, 且 D 也不存在了                -> LostDependencies (需人工确认)
 */
class DOODLEEDITOR_API FDoodleReferenceAudit
{
public:
	struct FEdge
	{
		FName Package;
		FName Dependency;

		bool operator==(const FEdge& Other) const
		{
			return Package == Other.Package && Dependency == Other.Dependency;
		}

		friend uint32 GetTypeHash(const FEdge& InEdge)
		{
			return HashCombine(GetTypeHash(InEdge.Package), GetTypeHash(InEdge.Dependency));
		}
	};

	struct FSnapshot
	{
		FDateTime TakenAt;
		FString RootPath;
		TArray<FEdge> HardEdges;
		TArray<FEdge> SoftEdges;
		TSet<FName> Packages;

		bool Save(const FString& Filename) const;
		static bool Load(const FString& Filename, FSnapshot& Out);
	};

	struct FDiffEntry
	{
		FName Package;
		FName Dependency;
		FName RetargetedTo;
		FString Note;
	};

	struct FDiffOptions
	{
		/** 旧包名 -> 新包名 (本次整理真正发生的移动) */
		TMap<FName, FName> MoveMap;

		/**
		 * true  = 用磁盘/资产注册表判断依赖包是否仍存在 (真实运行时的默认值)
		 * false = 只用 After.Packages 判断 (便于单元测试构造合成快照)
		 */
		bool bCheckRegistryForExistence = true;
	};

	struct FDiffResult
	{
		/** 真·引用丢失 */
		TArray<FDiffEntry> LostEdges;
		/** 依赖包本身也消失了, 需人工确认 */
		TArray<FDiffEntry> LostDependencies;
		/** 依赖被搬走且引用正确跟着改了 (正常) */
		TArray<FDiffEntry> RetargetedEdges;
		/** 新增的边 */
		TArray<FDiffEntry> AddedEdges;

		int32 NumEdgesBefore = 0;
		int32 NumEdgesAfter = 0;
		int32 NumPackagesBefore = 0;
		int32 NumPackagesAfter = 0;

		bool HasRealLoss() const { return LostEdges.Num() > 0; }
		bool HasAnyLoss() const { return LostEdges.Num() > 0 || LostDependencies.Num() > 0; }
		bool HasAnyChange() const
		{
			return LostEdges.Num() > 0 || LostDependencies.Num() > 0
				|| RetargetedEdges.Num() > 0 || AddedEdges.Num() > 0;
		}

		FString ToText(int32 MaxEntries = 200) const;
		FString ToCsv() const;
	};

	/** 采集 RootPath 下所有包的引用边 (硬引用 + 软引用) */
	static bool Capture(const FString& RootPath, FSnapshot& Out);

	static FDiffResult Diff(const FSnapshot& Before, const FSnapshot& After,
		const FDiffOptions& Options = FDiffOptions());

	/** 扫描「依赖包在磁盘与注册表都找不到」的边 (只看注册表, 看不见未保存的包) */
	static TArray<FDiffEntry> FindDanglingReferences(const FString& RootPath);

	/**
	 * 扫描「已加载对象里指向已不存在路径的软引用」。
	 *
	 * 为什么需要它: 引擎的引用修复 (FAssetRenameManager::PopulateAssetReferencers) 只用
	 * IAssetRegistry::GetReferencers 取引用者集合, 而注册表的依赖数据来自磁盘;
	 * 未保存(脏)的包不在里面。引擎只在 bOnlyFixSoftReferences == true 时才会额外把
	 * 脏包加进修复集合 (AssetRenameManager.cpp:827-868), 而整理走的都是 false。
	 * 结果: 脏包里的 TSoftObjectPtr / FSoftObjectPath 不会被改写, 保存后就成了死路径。
	 * 硬引用不受影响 —— UObject* 会跟着内存里的重命名走, 软引用不会。
	 *
	 * @param bDirtyPackagesOnly true = 只查脏包 / 未保存的包 (快, 适合每次操作后自动跑)
	 * @param MaxEntries        最多返回多少条
	 */
	static TArray<FDiffEntry> FindDanglingSoftReferences(const FString& RootPath,
		bool bDirtyPackagesOnly = true, int32 MaxEntries = 2000);

	/**
	 * 【修复】把已加载对象里仍指向旧路径的软引用原地改写成新路径。
	 *
	 * 这是 FindDanglingSoftReferences 的「纠正」对偶面: 引擎的 FAssetRenameManager 只修
	 * 注册表知道的引用者, 未保存的包会被漏掉 (见 .cpp 里的长注释)。这里不管注册表,
	 * 直接把内存里所有软引用扫一遍, 命中 MoveMap 的旧包名就改写成新包名 ——
	 * 引擎已经改好的那些指向新路径, 不会命中, 所以是幂等且不重复的。
	 *
	 * 被改写的包会被 MarkPackageDirty(), 否则改动不会落盘。
	 *
	 * @param MoveMap             旧包名 -> 新包名 (FDoodleOrganizeReport::MovedPackages)
	 * @param RootPath            只处理这个包路径下的引用者
	 * @param OutTouchedPackages  被改写的引用者包名 (需要用户保存)
	 * @return 改写的软引用条数
	 */
	static int32 RetargetSoftReferences(const TMap<FName, FName>& MoveMap, const FString& RootPath,
		TArray<FString>& OutTouchedPackages);

	/**
	 * 软引用遍历器的覆盖统计 (只读)。
	 *
	 * 用来回答「补修到底有没有生效」: 如果 NumSoftPathsSeen 是 0, 那 RetargetSoftReferences
	 * 必然是空操作。但要注意区分两种 0:
	 *   - NumObjectsVisited == 0  => 内存里根本没加载 /Game 的东西 (headless 跑就是这样),
	 *                                不代表遍历器坏了;
	 *   - NumObjectsVisited > 0 但 NumSoftPropertiesSeen == 0 => 那些对象真的没有软引用属性;
	 *   - NumSoftPropertiesSeen > 0 但 NumSoftPathsSeen == 0   => 软引用值都为空, 也是正常的。
	 */
	struct FSoftScanStats
	{
		/** 遍历到的包数 (已加载且命中根路径) */
		int32 NumPackagesVisited = 0;
		/** 遍历到的对象数 */
		int32 NumObjectsVisited = 0;
		/** 命中的软引用属性个数 (TSoftObjectPtr / FSoftObjectPath, 不管值是否为空) */
		int32 NumSoftPropertiesSeen = 0;
		/** 有效的、且指向根路径下的软引用路径条数 (= RetargetSoftReferences 能覆盖的范围) */
		int32 NumSoftPathsSeen = 0;
	};

	/** 统计软引用遍历器的覆盖范围 (只读, 不修改任何东西) */
	static FSoftScanStats ScanSoftReferences(const FString& RootPath, bool bDirtyPackagesOnly = false);

	/** Saved/DoodleOrganize (不存在时自动创建) */
	static FString GetReportDirectory();

	/** Saved/DoodleOrganize/refs_<Tag>_<时间戳>.json */
	static FString MakeSnapshotFilename(const TCHAR* Tag);

	/** 把文本写进报告目录, 返回实际写入的完整路径 (失败返回空串) */
	static FString WriteReportFile(const FString& BaseName, const FString& Content);
};
