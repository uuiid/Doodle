// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

/**
 * 文件整理操作的总体结果。
 * 说明: 这里刻意不使用 UENUM/USTRUCT —— 这些类型只在 C++ 内部使用,
 * 不需要反射, 可以避免 UHT 往返并让头文件可以被任意模块包含。
 */
enum class EDoodleOrganizeResult : uint8
{
	/** 全部成功 (可能带 warning) */
	Success,
	/** 部分成功 */
	PartialSuccess,
	/** 全部失败 */
	Failed,
	/** 用户取消 */
	Cancelled,
	/** 入参非法 (例如目标文件夹为空) */
	InvalidInput,
};

/**
 * 一次整理操作的报告。所有写操作都必须返回它, 不允许静默失败。
 */
struct DOODLEEDITOR_API FDoodleOrganizeReport
{
	EDoodleOrganizeResult Result = EDoodleOrganizeResult::Success;

	/** 操作名, 例如 "OrganizeSelected" */
	FString Operation;

	int32 NumRequested = 0;
	int32 NumMoved = 0;
	int32 NumRenamed = 0;
	int32 NumSkipped = 0;
	int32 NumFailed = 0;

	/** 旧路径仍然留有重定向器兜底的资产数量 (引用安全网指标) */
	int32 NumRedirectorsLeft = 0;

	/**
	 * 引擎漏掉、由 FDoodleReferenceAudit::RetargetSoftReferences 补修的软引用条数。
	 *
	 * 引擎的引用修复只用资产注册表的引用者集合, 未保存的包不在里面;
	 * 本地资产移动时又不会留重定向器, 所以这些软引用如果没人补修, 保存后就是死路径。
	 */
	int32 NumRetargetedSoftReferences = 0;

	/** 被补修过的引用者包名 (已被标脏, 需要用户保存才会落盘) */
	TArray<FString> RetargetedPackages;

	/** 整理前被自动保存的脏包数量 (先落盘 -> 依赖进注册表 -> 引擎那趟也能修) */
	int32 NumDirtyPackagesSaved = 0;

	/** 被自动保存的脏包名 */
	TArray<FString> SavedDirtyPackages;

	/** 第二趟 (bLoadAllPackages=true) 是否真的跑了 */
	bool bRanLoadAllPackagesPass = false;

	/** 整理后被删除的重定向器数量 */
	int32 NumRedirectorsDeleted = 0;

	/** "src -> dst : reason" */
	TArray<FString> Failures;
	/** "path : reason" */
	TArray<FString> Skipped;
	TArray<FString> Warnings;

	/**
	 * 本次操作真正发生的包移动 (旧包名 -> 新包名)。
	 * 诊断页会把它交给 FDoodleReferenceAudit::Diff, 用来判定
	 * "依赖被搬走了, 但引用没跟着改" 这种真·引用丢失。
	 */
	TMap<FName, FName> MovedPackages;

	double ElapsedSeconds = 0.0;

	bool HasFailures() const { return NumFailed > 0; }
	bool WasCancelled() const { return Result == EDoodleOrganizeResult::Cancelled; }
	bool HasProblems() const
	{
		return NumFailed > 0
			|| Result == EDoodleOrganizeResult::Failed
			|| Result == EDoodleOrganizeResult::PartialSuccess
			|| Result == EDoodleOrganizeResult::Cancelled
			|| Result == EDoodleOrganizeResult::InvalidInput;
	}

	void AddFailure(const FString& InFailure)
	{
		++NumFailed;
		if (Failures.Num() < 5000)
		{
			Failures.Add(InFailure);
		}
	}

	void AddSkip(const FString& InReason)
	{
		++NumSkipped;
		if (Skipped.Num() < 5000)
		{
			Skipped.Add(InReason);
		}
	}

	void AddWarning(const FString& InWarning)
	{
		if (Warnings.Num() < 5000)
		{
			Warnings.Add(InWarning);
		}
	}

	/** 根据计数推导 Result。显式设置过 Cancelled/InvalidInput 的会被保留。 */
	void Finalize()
	{
		if (Result == EDoodleOrganizeResult::Cancelled || Result == EDoodleOrganizeResult::InvalidInput)
		{
			return;
		}
		if (NumFailed == 0)
		{
			Result = EDoodleOrganizeResult::Success;
		}
		else if ((NumMoved + NumRenamed) > 0)
		{
			Result = EDoodleOrganizeResult::PartialSuccess;
		}
		else
		{
			Result = EDoodleOrganizeResult::Failed;
		}
	}

	static const TCHAR* ResultToString(EDoodleOrganizeResult InResult)
	{
		switch (InResult)
		{
		case EDoodleOrganizeResult::Success:       return TEXT("成功");
		case EDoodleOrganizeResult::PartialSuccess:return TEXT("部分成功");
		case EDoodleOrganizeResult::Failed:        return TEXT("失败");
		case EDoodleOrganizeResult::Cancelled:     return TEXT("已取消");
		case EDoodleOrganizeResult::InvalidInput:  return TEXT("参数非法");
		default:                                   return TEXT("未知");
		}
	}

	FText ToSummaryText() const
	{
		if (Result == EDoodleOrganizeResult::InvalidInput || Result == EDoodleOrganizeResult::Cancelled)
		{
			return FText::FromString(FString::Printf(TEXT("[%s] %s"), *Operation, ResultToString(Result)));
		}

		FString Out = FString::Printf(
			TEXT("[%s] %s  请求 %d / 移动 %d / 改名 %d / 跳过 %d / 失败 %d / 保留重定向器 %d"),
			*Operation, ResultToString(Result),
			NumRequested, NumMoved, NumRenamed, NumSkipped, NumFailed, NumRedirectorsLeft);

		// 只在真的发生时显示, 免得报告条太长
		if (NumDirtyPackagesSaved > 0)
		{
			Out += FString::Printf(TEXT(" / 保存脏包 %d"), NumDirtyPackagesSaved);
		}
		if (NumRetargetedSoftReferences > 0)
		{
			Out += FString::Printf(TEXT(" / 补修软引用 %d"), NumRetargetedSoftReferences);
		}
		if (bRanLoadAllPackagesPass)
		{
			Out += TEXT(" / 已加载全部软引用者");
		}
		if (NumRedirectorsDeleted > 0)
		{
			Out += FString::Printf(TEXT(" / 删除重定向器 %d"), NumRedirectorsDeleted);
		}

		Out += FString::Printf(TEXT("  (%.2fs)"), ElapsedSeconds);
		return FText::FromString(Out);
	}

	FString ToDetailedText() const
	{
		FString Out = ToSummaryText().ToString();
		Out += TEXT("\n");
		if (Failures.Num() > 0)
		{
			Out += FString::Printf(TEXT("\n失败 (%d):\n"), Failures.Num());
			for (const FString& Item : Failures)
			{
				Out += TEXT("  ") + Item + TEXT("\n");
			}
		}
		if (Warnings.Num() > 0)
		{
			Out += FString::Printf(TEXT("\n警告 (%d):\n"), Warnings.Num());
			for (const FString& Item : Warnings)
			{
				Out += TEXT("  ") + Item + TEXT("\n");
			}
		}
		if (Skipped.Num() > 0)
		{
			Out += FString::Printf(TEXT("\n跳过 (%d):\n"), Skipped.Num());
			for (const FString& Item : Skipped)
			{
				Out += TEXT("  ") + Item + TEXT("\n");
			}
		}
		return Out;
	}
};

/** 单条移动请求 */
struct DOODLEEDITOR_API FDoodleMoveRequest
{
	FAssetData Asset;

	/** 目标长包路径, 例如 /Game/Foo/Tex */
	FString DestinationFolder;

	/** 目标资产名; 为空表示沿用当前 AssetName */
	FString DestinationName;

	/** 目标已存在时是否自动生成唯一名 */
	bool bRenameOnConflict = true;

	bool IsValid() const { return Asset.IsValid() && !DestinationFolder.IsEmpty(); }
};

/** 单条移动结果 */
struct DOODLEEDITOR_API FDoodleMoveOutcome
{
	FString SourcePackage;
	FString FinalPackage;
	FString FailureReason;

	bool bMoved = false;
	bool bRenamedForConflict = false;

	/** 旧路径上留下了重定向器 (引用安全网) */
	bool bRedirectorLeft = false;

	/** 资产本来就在目标目录, 未做任何改动 */
	bool bSkippedBecauseInPlace = false;

	bool bSucceeded() const { return bMoved || bSkippedBecauseInPlace; }
};
