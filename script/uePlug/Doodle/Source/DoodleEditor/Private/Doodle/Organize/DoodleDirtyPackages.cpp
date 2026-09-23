// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/DoodleDirtyPackages.h"

#include "Doodle/Organize/DoodleAssetPathUtils.h"

#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace DoodleOrganize
{
	TArray<UPackage*> FindDirtyProjectPackages(const FString& RootPath)
	{
		TArray<UPackage*> Result;

		if (RootPath.IsEmpty())
		{
			return Result;
		}

		// 用编辑器自己的脏包枚举 —— 它同时覆盖世界包和内容包
		// (AssetRenameManager.cpp:828-829 用的也是这两个)
		TArray<UPackage*> DirtyPackages;
		FEditorFileUtils::GetDirtyWorldPackages(DirtyPackages);
		FEditorFileUtils::GetDirtyContentPackages(DirtyPackages);

		for (UPackage* Package : DirtyPackages)
		{
			if (Package == nullptr)
			{
				continue;
			}

			const FString PackageName = Package->GetName();
			if (!PackageName.StartsWith(RootPath, ESearchCase::CaseSensitive))
			{
				continue;
			}

			// 编译期包 / PIE 包不是资产, 也不该被保存
			if (Package->HasAnyPackageFlags((uint32)(PKG_CompiledIn | PKG_PlayInEditor)))
			{
				continue;
			}

			Result.AddUnique(Package);
		}

		// 名字排序, 让报告和确认框稳定可读
		Result.Sort([](const UPackage& A, const UPackage& B)
		{
			return A.GetName().Compare(B.GetName(), ESearchCase::CaseSensitive) < 0;
		});

		return Result;
	}

	TArray<FString> GetPackageNames(const TArray<UPackage*>& Packages)
	{
		TArray<FString> Names;
		Names.Reserve(Packages.Num());

		for (const UPackage* Package : Packages)
		{
			if (Package != nullptr)
			{
				Names.AddUnique(Package->GetName());
			}
		}

		Names.Sort([](const FString& A, const FString& B)
		{
			return A.Compare(B, ESearchCase::CaseSensitive) < 0;
		});

		return Names;
	}

	bool SavePackages(const TArray<UPackage*>& Packages)
	{
		if (Packages.Num() == 0)
		{
			return true;
		}

		// bOnlyDirty = true: 只保存真正脏了的那些
		// 注意: SavePackages 属于 UEditorLoadingAndSavingUtils, 不是 FEditorFileUtils
		return UEditorLoadingAndSavingUtils::SavePackages(Packages, /*bOnlyDirty*/ true);
	}

	FDoodleOrganizeReport SaveDirtyPackagesBeforeOrganize(
		const FString& RootPath,
		bool bAsk,
		TFunctionRef<bool(const TArray<FString>&)> AskFn,
		bool bSave)
	{
		FDoodleOrganizeReport Report;
		Report.Operation = TEXT("SaveDirtyPackagesBeforeOrganize");

		const TArray<UPackage*> DirtyPackages = FindDirtyProjectPackages(RootPath);
		Report.NumRequested = DirtyPackages.Num();

		if (DirtyPackages.Num() == 0)
		{
			Report.Finalize();
			return Report;
		}

		const TArray<FString> DirtyNames = GetPackageNames(DirtyPackages);

		if (bAsk && !AskFn(DirtyNames))
		{
			// 用户拒绝保存 => 调用方应中止整理
			Report.Result = EDoodleOrganizeResult::Cancelled;
			Report.AddWarning(FString::Printf(
				TEXT("检测到 %d 个未保存的包, 用户选择不保存。"), DirtyNames.Num()));
			Report.Finalize();
			return Report;
		}

		if (!bSave)
		{
			// 只检测不保存 (预览)
			for (const FString& Name : DirtyNames)
			{
				Report.AddWarning(FString::Printf(TEXT("未保存: %s"), *Name));
			}
			Report.Finalize();
			return Report;
		}

		const bool bAllSaved = SavePackages(DirtyPackages);

		if (bAllSaved)
		{
			Report.NumDirtyPackagesSaved = DirtyNames.Num();
			Report.SavedDirtyPackages = DirtyNames;
		}
		else
		{
			// 保存失败的包, 报告里说清楚 —— 整理仍然继续, 但引用风险变高
			Report.NumDirtyPackagesSaved = 0;
			Report.AddWarning(FString::Printf(
				TEXT("有 %d 个未保存的包保存失败 (可能被版本控制锁住或只读), 它们的引用不会被引擎自动修复。"),
				DirtyNames.Num()));
			for (const FString& Name : DirtyNames)
			{
				Report.AddWarning(FString::Printf(TEXT("保存失败: %s"), *Name));
			}
		}

		Report.Finalize();
		return Report;
	}
}
