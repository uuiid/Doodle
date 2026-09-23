// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/DoodleAssetOrganizer.h"

#include "Doodle/Organize/DoodleAssetPathUtils.h"
#include "Doodle/Organize/DoodleOrganizeRules.h"
#include "Doodle/Organize/DoodleOrganizeSettings.h"
#include "Doodle/Organize/DoodleRedirectFixup.h"
#include "Doodle/Organize/DoodleReferenceAudit.h"
#include "Doodle/ResizeTexture.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "Engine/Level.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformTime.h"
#include "IAssetTools.h"
#include "Materials/MaterialInterface.h"
#include "Misc/NamePermissionList.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Settings/EditorLoadingSavingSettings.h"
#include "Subsystems/EditorAssetSubsystem.h"

using namespace UE::AssetRegistry;

#define LOCTEXT_NAMESPACE "DoodleOrganize"

namespace
{
	UEditorAssetSubsystem* GetEditorAssetSubsystem()
	{
		return GEditor ? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>() : nullptr;
	}

	IAssetRegistry* GetAssetRegistry()
	{
		if (!FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
		{
			return nullptr;
		}
		return &FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	}

	/**
	 * 把 From 的计数/明细合并进 Into。
	 * 注意: 不合并 NumRequested (调用方自己决定请求总数)。
	 */
	void MergeReport(FDoodleOrganizeReport& Into, const FDoodleOrganizeReport& From)
	{
		Into.NumMoved += From.NumMoved;
		Into.NumRenamed += From.NumRenamed;
		Into.NumSkipped += From.NumSkipped;
		Into.NumFailed += From.NumFailed;
		Into.NumRedirectorsLeft += From.NumRedirectorsLeft;
		Into.NumRetargetedSoftReferences += From.NumRetargetedSoftReferences;
		Into.RetargetedPackages.Append(From.RetargetedPackages);
		Into.Failures.Append(From.Failures);
		Into.Skipped.Append(From.Skipped);
		Into.Warnings.Append(From.Warnings);
		Into.ElapsedSeconds += From.ElapsedSeconds;
		for (const TPair<FName, FName>& Pair : From.MovedPackages)
		{
			Into.MovedPackages.Add(Pair.Key, Pair.Value);
		}
	}

	/** 从注册表取某个包里第一个「非重定向器且是指定类型子类」的资产 */
	FAssetData FindFirstAssetOfClass(IAssetRegistry& Registry, FName PackageName, const UClass* RequiredBaseClass)
	{
		TArray<FAssetData> Assets;
		Registry.GetAssetsByPackageName(PackageName, Assets);
		for (const FAssetData& Asset : Assets)
		{
			if (!Asset.IsValid() || DoodleOrganize::IsRedirector(Asset))
			{
				continue;
			}
			const UClass* AssetClass = Asset.GetClass();
			if (AssetClass != nullptr && (RequiredBaseClass == nullptr || AssetClass->IsChildOf(RequiredBaseClass)))
			{
				return Asset;
			}
		}
		return FAssetData();
	}

	/** 取 From 下所有文件的 (相对路径, 绝对路径) */
	void CollectFilesRelativeTo(const FString& From, TArray<TPair<FString, FString>>& OutFiles)
	{
		IFileManager& FileManager = IFileManager::Get();

		FString Prefix = From;
		FPaths::NormalizeDirectoryName(Prefix);
		Prefix += TEXT("/");

		TArray<FString> Files;
		FileManager.FindFilesRecursive(Files, *From, TEXT("*"), /*Files*/ true, /*Directories*/ false);

		OutFiles.Reserve(Files.Num());
		for (const FString& File : Files)
		{
			if (!File.StartsWith(Prefix, ESearchCase::IgnoreCase))
			{
				continue;
			}
			OutFiles.Emplace(File.RightChop(Prefix.Len()), File);
		}
	}

	/**
	 * 递归搬目录。
	 * IPlatformFile / IFileManager 都没有「移动目录」的 API (IFileManager::Move 只搬文件),
	 * 所以这里自己逐文件搬, 全部成功后才删掉旧目录。
	 */
	bool MoveDirectoryRecursive(const FString& From, const FString& To)
	{
		IFileManager& FileManager = IFileManager::Get();
		if (!FileManager.DirectoryExists(*From))
		{
			return false;
		}

		FileManager.MakeDirectory(*To, /*Tree*/ true);

		TArray<TPair<FString, FString>> Files;
		CollectFilesRelativeTo(From, Files);

		bool bAllMoved = true;
		for (const TPair<FString, FString>& File : Files)
		{
			const FString Destination = FPaths::Combine(To, File.Key);
			FileManager.MakeDirectory(*FPaths::GetPath(Destination), /*Tree*/ true);
			if (!FileManager.Move(*Destination, *File.Value, /*Replace*/ true))
			{
				bAllMoved = false;
			}
		}

		if (bAllMoved)
		{
			FileManager.DeleteDirectory(*From, /*RequireExists*/ false, /*Tree*/ true);
		}
		return bAllMoved;
	}
}

FDoodleAssetOrganizer::FDoodleAssetOrganizer() = default;

// ---------------------------------------------------------------------------
// 枚举与工具
// ---------------------------------------------------------------------------

void FDoodleAssetOrganizer::EnumerateAssets(const FString& RootPath, const UClass* RequiredBaseClass,
	TArray<FAssetData>& OutAssets)
{
	OutAssets.Reset();

	IAssetRegistry* Registry = GetAssetRegistry();
	if (Registry == nullptr || RootPath.IsEmpty())
	{
		return;
	}

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName(*RootPath));
	if (RequiredBaseClass != nullptr)
	{
		Filter.ClassPaths.Add(RequiredBaseClass->GetClassPathName());
	}

	Registry->GetAssets(Filter, OutAssets);
}

int32 FDoodleAssetOrganizer::CountWorldsIn(const TArray<FAssetData>& Assets)
{
	int32 Count = 0;
	for (const FAssetData& Asset : Assets)
	{
		const UClass* AssetClass = Asset.GetClass();
		if (AssetClass != nullptr && AssetClass->IsChildOf(UWorld::StaticClass()))
		{
			++Count;
		}
	}
	return Count;
}

bool FDoodleAssetOrganizer::CheckWritable(const FString& Folder, FString& OutReason)
{
	OutReason.Reset();

	if (!FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
	{
		OutReason = TEXT("AssetTools 模块未加载");
		return false;
	}

	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	if (!AssetTools.GetWritableFolderPermissionList()->PassesStartsWithFilter(Folder))
	{
		OutReason = FString::Printf(TEXT("目标目录 %s 不在可写白名单内"), *Folder);
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// 移动
// ---------------------------------------------------------------------------

FDoodleMoveOutcome FDoodleAssetOrganizer::MoveOneAsset(const FDoodleMoveRequest& Request, bool bUseDialog)
{
	TArray<FDoodleMoveRequest> Requests;
	Requests.Add(Request);

	TArray<FDoodleMoveOutcome> Outcomes;
	MoveAssetsInternal(Requests, bUseDialog, &Outcomes);

	if (Outcomes.Num() > 0)
	{
		return Outcomes[0];
	}

	FDoodleMoveOutcome Outcome;
	Outcome.SourcePackage = Request.Asset.PackageName.ToString();
	Outcome.FailureReason = TEXT("未产生移动结果");
	return Outcome;
}

FDoodleOrganizeReport FDoodleAssetOrganizer::MoveAssets(TArrayView<const FDoodleMoveRequest> Requests, bool bUseDialog)
{
	return MoveAssetsInternal(Requests, bUseDialog, nullptr);
}

FDoodleOrganizeReport FDoodleAssetOrganizer::MoveAssetsInternal(TArrayView<const FDoodleMoveRequest> Requests,
	bool bUseDialog, TArray<FDoodleMoveOutcome>* OutOutcomes)
{
	const double StartTime = FPlatformTime::Seconds();

	FDoodleOrganizeReport Report;
	Report.Operation = bUseDialog ? TEXT("MoveAssets(Dialog)") : TEXT("MoveAssets(Batch)");
	Report.NumRequested = Requests.Num();

	if (OutOutcomes != nullptr)
	{
		OutOutcomes->Reset();
	}

	auto RecordOutcome = [OutOutcomes](FDoodleMoveOutcome&& Outcome)
	{
		if (OutOutcomes != nullptr)
		{
			OutOutcomes->Add(MoveTemp(Outcome));
		}
	};

	IAssetRegistry* Registry = GetAssetRegistry();
	if (Registry == nullptr)
	{
		Report.Result = EDoodleOrganizeResult::Failed;
		Report.AddFailure(TEXT("AssetRegistry 模块未加载"));
		Report.ElapsedSeconds = FPlatformTime::Seconds() - StartTime;
		return Report;
	}

	// 注册表还在扫描时, 引擎会返回 Pending 并吞掉结果, 这里直接拒绝执行
	if (Registry->IsLoadingAssets())
	{
		Report.Result = EDoodleOrganizeResult::Failed;
		Report.AddFailure(TEXT("资产注册表仍在扫描中, 请等扫描结束后重试"));
		Report.ElapsedSeconds = FPlatformTime::Seconds() - StartTime;
		return Report;
	}

	if (!FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
	{
		Report.Result = EDoodleOrganizeResult::Failed;
		Report.AddFailure(TEXT("AssetTools 模块未加载"));
		Report.ElapsedSeconds = FPlatformTime::Seconds() - StartTime;
		return Report;
	}

	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	const TSharedRef<FPathPermissionList>& WritableFolders = AssetTools.GetWritableFolderPermissionList();

	// ---- 预处理: 跳过 / 求唯一名 / 权限检查 ----
	TArray<FAssetRenameData> RenameData;
	TArray<FString> SourcePackages;
	TArray<bool> RenamedForConflict;
	RenameData.Reserve(Requests.Num());
	SourcePackages.Reserve(Requests.Num());
	RenamedForConflict.Reserve(Requests.Num());

	for (const FDoodleMoveRequest& Request : Requests)
	{
		const FAssetData& Asset = Request.Asset;

		if (!Request.IsValid())
		{
			Report.AddFailure(TEXT("<无效请求> : 资产数据或目标目录为空"));
			continue;
		}

		const FString SourcePackage = Asset.PackageName.ToString();

		if (DoodleOrganize::IsRedirector(Asset))
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 重定向器不参与整理"), *SourcePackage));
			continue;
		}

		FString TargetFolder = Request.DestinationFolder;
		TargetFolder.RemoveFromEnd(TEXT("/"));

		// 关键修正: 资产本来就在目标目录时直接跳过。
		// 原实现用 DoesAssetExist(目标路径) 判断, 命中的永远是"自己", 于是必然改名成 Name_1 / Name_2。
		if (DoodleOrganize::IsUnderPackagePath(SourcePackage, TargetFolder))
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 已经在目标目录, 未做改动"), *SourcePackage));

			FDoodleMoveOutcome Outcome;
			Outcome.SourcePackage = SourcePackage;
			Outcome.FinalPackage = SourcePackage;
			Outcome.bSkippedBecauseInPlace = true;
			RecordOutcome(MoveTemp(Outcome));
			continue;
		}

		const FString DesiredName = Request.DestinationName.IsEmpty()
			? Asset.AssetName.ToString()
			: Request.DestinationName;

		FString FinalFolder;
		FString FinalName;
		bool bRenamedForConflict = false;

		if (Request.bRenameOnConflict)
		{
			const FString DesiredPackage = DoodleOrganize::CombinePackagePath(TargetFolder, DesiredName);
			if (!DoodleOrganize::TryMakeUniqueAssetPath(TargetFolder, DesiredName, FinalFolder, FinalName))
			{
				Report.AddFailure(FString::Printf(TEXT("%s -> %s : 无法生成唯一资产名"), *SourcePackage, *TargetFolder));
				continue;
			}
			bRenamedForConflict = !DoodleOrganize::CombinePackagePath(FinalFolder, FinalName)
				.Equals(DesiredPackage, ESearchCase::CaseSensitive);
		}
		else
		{
			const FString DesiredPackage = DoodleOrganize::CombinePackagePath(TargetFolder, DesiredName);
			if (!DoodleOrganize::IsAssetPathFree(DesiredPackage))
			{
				Report.AddFailure(FString::Printf(TEXT("%s -> %s : 目标已存在"), *SourcePackage, *DesiredPackage));
				continue;
			}
			FinalFolder = TargetFolder;
			FinalName = DesiredName;
		}

		if (!WritableFolders->PassesStartsWithFilter(FinalFolder))
		{
			AssetTools.NotifyBlockedByWritableFolderFilter();
			Report.AddFailure(FString::Printf(TEXT("%s -> %s : 目标目录不在可写白名单内"), *SourcePackage, *FinalFolder));
			continue;
		}

		// 注意: 必须在这里就取到对象。原实现在移动之后才 GetAsset(),
		// 那时旧包已经被删, 拿到的是 nullptr 或重定向器。
		UObject* AssetObject = Asset.GetAsset();
		if (AssetObject == nullptr)
		{
			Report.AddFailure(FString::Printf(TEXT("%s : 无法加载资产对象"), *SourcePackage));
			continue;
		}

		RenameData.Emplace(AssetObject, FinalFolder, FinalName,
			/*bOnlyFixSoftReferences*/ false, /*bAlsoRenameLocalizedVariants*/ true);
		SourcePackages.Add(SourcePackage);
		RenamedForConflict.Add(bRenamedForConflict);
	}

	if (RenameData.Num() == 0)
	{
		Report.Finalize();
		Report.ElapsedSeconds = FPlatformTime::Seconds() - StartTime;
		return Report;
	}

	// ---- 一次性提交 (修 E21/E34) ----
	const UEditorLoadingSavingSettings* LoadingSettings = GetDefault<UEditorLoadingSavingSettings>();
	const bool bAutoCheckout = LoadingSettings != nullptr && LoadingSettings->GetAutomaticallyCheckoutOnAssetModification();

	// RenameAssets 返回 bool, RenameAssetsWithDialog 返回 EAssetRenameResult —— 不能写成一个三目
	EAssetRenameResult RenameResult = EAssetRenameResult::Success;
	if (bUseDialog)
	{
		RenameResult = AssetTools.RenameAssetsWithDialog(RenameData, bAutoCheckout);
	}
	else
	{
		RenameResult = AssetTools.RenameAssets(RenameData) ? EAssetRenameResult::Success : EAssetRenameResult::Failure;
	}

	if (RenameResult == EAssetRenameResult::Failure)
	{
		for (const FString& SourcePackage : SourcePackages)
		{
			Report.AddFailure(FString::Printf(TEXT("%s : 引擎重命名失败 (EAssetRenameResult::Failure)"), *SourcePackage));
		}
		Report.Finalize();
		Report.ElapsedSeconds = FPlatformTime::Seconds() - StartTime;
		return Report;
	}

	if (RenameResult == EAssetRenameResult::Pending)
	{
		Report.AddWarning(TEXT("引擎返回 Pending (资产发现尚未完成), 请等扫描结束后重试"));
	}

	// ---- 后置: 用新路径重新解析, 并检查旧路径是否留了兜底 ----
	for (int32 Index = 0; Index < SourcePackages.Num(); ++Index)
	{
		const FString& SourcePackage = SourcePackages[Index];
		const FAssetRenameData& Data = RenameData[Index];
		const FString FinalPackage = DoodleOrganize::CombinePackagePath(Data.NewPackagePath, Data.NewName);

		FDoodleMoveOutcome Outcome;
		Outcome.SourcePackage = SourcePackage;
		Outcome.FinalPackage = FinalPackage;
		Outcome.bMoved = true;
		Outcome.bRenamedForConflict = RenamedForConflict[Index];

		// 旧路径上还有东西 => 引擎留了重定向器 (引用安全网)
		if (DoodleOrganize::DoesAssetExistOnDisk(SourcePackage)
			|| DoodleOrganize::DoesAssetExistInRegistry(SourcePackage, /*bIncludeRedirectors*/ true))
		{
			Outcome.bRedirectorLeft = true;
			++Report.NumRedirectorsLeft;
		}

		if (Outcome.bRenamedForConflict)
		{
			++Report.NumRenamed;
		}
		++Report.NumMoved;
		Report.MovedPackages.Add(FName(*SourcePackage), FName(*FinalPackage));

		RecordOutcome(MoveTemp(Outcome));
	}

	// 地图被搬动后, 它的 __ExternalActors__ 目录必须跟着走
	HandleWorldExternalActors(Report);

	// ---- 核心修复: 补上引擎漏掉的软引用 ----
	//
	// FAssetRenameManager::PopulateAssetReferencers (AssetRenameManager.cpp:809-870) 的引用者
	// 集合完全来自 IAssetRegistry::GetReferencers, 也就是磁盘上的依赖图; 未保存(脏)的包不在里面。
	// 引擎确实有一段脏包兜底 (827-868 行), 但它被关在 if (bOnlyFixSoftReferences) 里,
	// 普通移动走的是 false —— 永远不执行。
	// 同时本地资产移动时引擎**不会**留重定向器 (971-999 行: "Only local assets should be renamed
	// without a redirector"), 所以旧路径直接消失。
	// 硬引用没事 (UObject* 跟着内存里的重命名走), 软引用是字符串路径, 保存后就成了死路径。
	//
	// 这里不管注册表, 直接把内存里所有软引用扫一遍, 命中 MoveMap 的就改写成新路径。
	// 引擎已经改好的那些指向新路径, 不会命中, 所以不会重复改。
	if (UDoodleOrganizeSettings::Get().bRetargetSoftReferencesAfterMove && Report.MovedPackages.Num() > 0)
	{
		TArray<FString> TouchedPackages;
		const int32 NumRetargeted = FDoodleReferenceAudit::RetargetSoftReferences(
			Report.MovedPackages, DoodleOrganize::GetGameRootPath(), TouchedPackages);

		Report.NumRetargetedSoftReferences = NumRetargeted;
		Report.RetargetedPackages = TouchedPackages;

		if (NumRetargeted > 0)
		{
			Report.AddWarning(FString::Printf(
				TEXT("补修了 %d 条引擎漏掉的软引用 (分布在 %d 个包里)。")
				TEXT("这些包已被标记为已修改, 请保存它们, 否则修改不会落盘。"),
				NumRetargeted, TouchedPackages.Num()));

			for (int32 Index = 0; Index < TouchedPackages.Num() && Index < 20; ++Index)
			{
				Report.AddWarning(FString::Printf(TEXT("  待保存: %s"), *TouchedPackages[Index]));
			}
		}
	}

	// 旧路径上确实存在重定向器时, 才做一次修复 (默认不删除)
	const FDoodleOrganizeReport FixupReport = FixupRedirectorsForMovedPackages(Report.MovedPackages);
	MergeReport(Report, FixupReport);

	Report.Finalize();
	Report.ElapsedSeconds = FPlatformTime::Seconds() - StartTime;
	return Report;
}

// ---------------------------------------------------------------------------
// 地图的 __ExternalActors__ 目录
// ---------------------------------------------------------------------------

void FDoodleAssetOrganizer::HandleWorldExternalActors(FDoodleOrganizeReport& Report) const
{
	if (Report.MovedPackages.Num() == 0)
	{
		return;
	}

	const UDoodleOrganizeSettings& Settings = UDoodleOrganizeSettings::Get();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	for (const TPair<FName, FName>& Pair : Report.MovedPackages)
	{
		const FString OldExternalActors = ULevel::GetExternalActorsPath(Pair.Key.ToString());
		const FString NewExternalActors = ULevel::GetExternalActorsPath(Pair.Value.ToString());
		if (OldExternalActors.IsEmpty() || NewExternalActors.IsEmpty() || OldExternalActors == NewExternalActors)
		{
			continue;
		}

		const FString OldDirectory = FPackageName::LongPackageNameToFilename(OldExternalActors, TEXT(""));
		const FString NewDirectory = FPackageName::LongPackageNameToFilename(NewExternalActors, TEXT(""));

		if (!PlatformFile.DirectoryExists(*OldDirectory))
		{
			// 不是 World Partition 地图 (或没有外部 actor), 无事可做
			continue;
		}

		if (!Settings.bMoveWorldExternalActors)
		{
			Report.AddWarning(FString::Printf(
				TEXT("地图 %s 已移动, 但 __ExternalActors__ 目录未搬迁 (设置里已关闭)。请手工移动: %s -> %s"),
				*Pair.Key.ToString(), *OldDirectory, *NewDirectory));
			continue;
		}

		if (PlatformFile.DirectoryExists(*NewDirectory))
		{
			Report.AddWarning(FString::Printf(
				TEXT("地图 %s 的目标 __ExternalActors__ 目录已存在, 未覆盖: %s"), *Pair.Value.ToString(), *NewDirectory));
			continue;
		}

		if (MoveDirectoryRecursive(OldDirectory, NewDirectory))
		{
			Report.AddWarning(FString::Printf(
				TEXT("已随地图搬迁 __ExternalActors__ 目录: %s -> %s"), *OldDirectory, *NewDirectory));

			if (IAssetRegistry* Registry = GetAssetRegistry())
			{
				Registry->ScanPathsSynchronous({ OldExternalActors, NewExternalActors }, /*bForceRescan*/ true);
			}
		}
		else
		{
			Report.AddFailure(FString::Printf(
				TEXT("地图 %s 的 __ExternalActors__ 目录搬迁失败, 请手工移动: %s -> %s"),
				*Pair.Key.ToString(), *OldDirectory, *NewDirectory));
		}
	}
}

FDoodleOrganizeReport FDoodleAssetOrganizer::FixupRedirectorsForMovedPackages(
	const TMap<FName, FName>& MovedPackages) const
{
	FDoodleOrganizeReport Report;
	Report.Operation = TEXT("FixupRedirectorsForMovedPackages");

	// 只收集「旧路径上确实存在重定向器」的包
	TArray<FString> RedirectorPaths;
	for (const TPair<FName, FName>& Pair : MovedPackages)
	{
		const FString OldPackage = Pair.Key.ToString();
		if (DoodleOrganize::DoesAssetExistInRegistry(OldPackage, /*bIncludeRedirectors*/ true))
		{
			RedirectorPaths.Append(DoodleOrganize::FindRedirectorsUnderPath(OldPackage));
		}
	}

	if (RedirectorPaths.Num() == 0)
	{
		Report.Finalize();
		return Report;
	}

	// 默认不删除重定向器 —— 删掉就少了旧路径这一层引用兜底
	return DoodleOrganize::FixupRedirectorsForPaths(RedirectorPaths,
		ERedirectFixupMode::LeaveFixedUpRedirectors);
}

// ---------------------------------------------------------------------------
// 按规则整理
// ---------------------------------------------------------------------------

void FDoodleAssetOrganizer::BuildOrganizeRequests(const TArray<FAssetData>& Assets, const FString& TargetRootFolder,
	const DoodleOrganize::FRuleSet& Rules, TArray<FDoodleMoveRequest>& OutRequests, FDoodleOrganizeReport& Report) const
{
	for (const FAssetData& Asset : Assets)
	{
		FString SubFolder;
		FString SkipReason;
		if (!DoodleOrganize::TryResolveSubFolder(Asset, Rules, SubFolder, SkipReason))
		{
			Report.AddSkip(FString::Printf(TEXT("%s : %s"), *Asset.PackageName.ToString(), *SkipReason));
			continue;
		}

		FDoodleMoveRequest Request;
		Request.Asset = Asset;
		Request.DestinationFolder = DoodleOrganize::CombinePackagePath(TargetRootFolder, SubFolder);
		Request.bRenameOnConflict = true;
		OutRequests.Add(MoveTemp(Request));
	}
}

FDoodleOrganizeReport FDoodleAssetOrganizer::OrganizeSelected(const TArray<FAssetData>& Assets,
	const FString& TargetFolderName)
{
	FDoodleOrganizeReport Report;
	Report.Operation = TEXT("OrganizeSelected");
	Report.NumRequested = Assets.Num();

	const FString FolderName = TargetFolderName.TrimStartAndEnd();
	if (FolderName.IsEmpty())
	{
		Report.Result = EDoodleOrganizeResult::InvalidInput;
		Report.AddFailure(TEXT("目标文件夹名称不能为空"));
		return Report;
	}

	if (Assets.Num() == 0)
	{
		Report.Result = EDoodleOrganizeResult::InvalidInput;
		Report.AddFailure(TEXT("请先在内容浏览器中选择要整理的资源"));
		return Report;
	}

	const UDoodleOrganizeSettings& Settings = UDoodleOrganizeSettings::Get();

	DoodleOrganize::FRuleSet Rules = DoodleOrganize::GetAssetRules();
	Rules.ExtraExcludedClasses = Settings.ExtraExcludedClasses;
	Rules.bIncludeOtherTypes = Settings.bIncludeOtherTypes;
	Rules.bIncludeWorlds = Settings.bIncludeWorlds;

	const FString TargetRoot = DoodleOrganize::CombinePackagePath(DoodleOrganize::GetGameRootPath(), FolderName);

	if (UEditorAssetSubsystem* EditorAssetSubsystem = GetEditorAssetSubsystem())
	{
		EditorAssetSubsystem->MakeDirectory(TargetRoot);
	}

	TArray<FDoodleMoveRequest> Requests;
	BuildOrganizeRequests(Assets, TargetRoot, Rules, Requests, Report);

	// 选中资源整理沿用内容浏览器的手感: 允许弹「About to load N assets」
	const FDoodleOrganizeReport MoveReport = MoveAssets(Requests, /*bUseDialog*/ true);
	MergeReport(Report, MoveReport);

	Report.Finalize();
	return Report;
}

FDoodleOrganizeReport FDoodleAssetOrganizer::OrganizeCharacterAssets(const FString& CharacterFolderName)
{
	FDoodleOrganizeReport Report;
	Report.Operation = TEXT("OrganizeCharacterAssets");

	const FString FolderName = CharacterFolderName.TrimStartAndEnd();
	if (FolderName.IsEmpty())
	{
		Report.Result = EDoodleOrganizeResult::InvalidInput;
		Report.AddFailure(TEXT("角色名称不能为空"));
		return Report;
	}

	const UDoodleOrganizeSettings& Settings = UDoodleOrganizeSettings::Get();

	DoodleOrganize::FRuleSet Rules = DoodleOrganize::GetCharacterRules();
	Rules.ExtraExcludedClasses = Settings.ExtraExcludedClasses;
	Rules.bIncludeOtherTypes = Settings.bIncludeOtherTypes;
	Rules.bIncludeWorlds = Settings.bIncludeWorlds;

	const FString TargetRoot = DoodleOrganize::CombinePackagePath(
		DoodleOrganize::CombinePackagePath(DoodleOrganize::GetGameRootPath(), TEXT("Character")), FolderName);

	TArray<FAssetData> AllAssets;
	EnumerateAssets(DoodleOrganize::GetGameRootPath(), nullptr, AllAssets);
	Report.NumRequested = AllAssets.Num();

	if (UEditorAssetSubsystem* EditorAssetSubsystem = GetEditorAssetSubsystem())
	{
		EditorAssetSubsystem->MakeDirectory(TargetRoot);
	}

	TArray<FDoodleMoveRequest> Requests;
	BuildOrganizeRequests(AllAssets, TargetRoot, Rules, Requests, Report);

	// 全量整理走批量模式: 不弹对话框, 也不做"保存整个 /Game"这种事
	const FDoodleOrganizeReport MoveReport = MoveAssets(Requests, /*bUseDialog*/ false);
	MergeReport(Report, MoveReport);

	Report.Finalize();
	return Report;
}

// ---------------------------------------------------------------------------
// 后缀
// ---------------------------------------------------------------------------

FDoodleOrganizeReport FDoodleAssetOrganizer::AddSuffix(const TArray<FAssetData>& Assets, const FString& Suffix)
{
	FDoodleOrganizeReport Report;
	Report.Operation = TEXT("AddSuffix");
	Report.NumRequested = Assets.Num();

	const FString TrimmedSuffix = Suffix.TrimStartAndEnd();
	if (TrimmedSuffix.IsEmpty())
	{
		Report.Result = EDoodleOrganizeResult::InvalidInput;
		Report.AddFailure(TEXT("请先填写后缀"));
		return Report;
	}

	TArray<FDoodleMoveRequest> Requests;
	for (const FAssetData& Asset : Assets)
	{
		FString NewName;
		if (!DoodleOrganize::MakeSuffixedName(Asset.AssetName.ToString(), TrimmedSuffix, NewName))
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 已经带后缀 _%s, 或名称无效"),
				*Asset.PackageName.ToString(), *TrimmedSuffix));
			continue;
		}

		FDoodleMoveRequest Request;
		Request.Asset = Asset;
		Request.DestinationFolder = Asset.PackagePath.ToString();
		Request.DestinationName = NewName;
		Request.bRenameOnConflict = false;
		Requests.Add(MoveTemp(Request));
	}

	const FDoodleOrganizeReport MoveReport = MoveAssets(Requests, /*bUseDialog*/ false);
	MergeReport(Report, MoveReport);

	Report.Finalize();
	return Report;
}

FDoodleOrganizeReport FDoodleAssetOrganizer::RemoveSuffix(const TArray<FAssetData>& Assets, const FString& Suffix)
{
	FDoodleOrganizeReport Report;
	Report.Operation = TEXT("RemoveSuffix");
	Report.NumRequested = Assets.Num();

	const UDoodleOrganizeSettings& Settings = UDoodleOrganizeSettings::Get();
	const bool bLegacy = Settings.bLegacyRemoveSuffixBehavior;

	const FString TrimmedSuffix = Suffix.TrimStartAndEnd();
	if (!bLegacy && TrimmedSuffix.IsEmpty())
	{
		Report.Result = EDoodleOrganizeResult::InvalidInput;
		Report.AddFailure(TEXT("请先填写后缀 (或在设置里开启「去后缀沿用旧行为」)"));
		return Report;
	}

	TArray<FDoodleMoveRequest> Requests;
	for (const FAssetData& Asset : Assets)
	{
		const FString OldName = Asset.AssetName.ToString();

		FString NewName;
		const bool bRenamed = bLegacy
			? DoodleOrganize::MakeUnsuffixedNameLegacy(OldName, NewName)
			: DoodleOrganize::MakeUnsuffixedName(OldName, TrimmedSuffix, NewName);

		if (!bRenamed)
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 没有匹配的后缀, 未做改动"), *Asset.PackageName.ToString()));
			continue;
		}

		FDoodleMoveRequest Request;
		Request.Asset = Asset;
		Request.DestinationFolder = Asset.PackagePath.ToString();
		Request.DestinationName = NewName;
		Request.bRenameOnConflict = false;
		Requests.Add(MoveTemp(Request));
	}

	const FDoodleOrganizeReport MoveReport = MoveAssets(Requests, /*bUseDialog*/ false);
	MergeReport(Report, MoveReport);

	Report.Finalize();
	return Report;
}

FDoodleOrganizeReport FDoodleAssetOrganizer::RenameAssetTo(const FAssetData& Asset, const FString& DesiredFolder,
	const FString& DesiredName)
{
	FDoodleOrganizeReport Report;
	Report.Operation = TEXT("RenameAssetTo");
	Report.NumRequested = 1;

	if (!Asset.IsValid() || DesiredFolder.IsEmpty() || DesiredName.IsEmpty())
	{
		Report.Result = EDoodleOrganizeResult::InvalidInput;
		Report.AddFailure(TEXT("改名参数不完整"));
		return Report;
	}

	TArray<FDoodleMoveRequest> Requests;
	FDoodleMoveRequest& Request = Requests.AddDefaulted_GetRef();
	Request.Asset = Asset;
	Request.DestinationFolder = DesiredFolder;
	Request.DestinationName = DesiredName;
	// 行内改名要求明确报"该命名已存在", 不自动改名
	Request.bRenameOnConflict = false;

	const FDoodleOrganizeReport MoveReport = MoveAssets(Requests, /*bUseDialog*/ false);
	MergeReport(Report, MoveReport);

	Report.Finalize();
	return Report;
}

// ---------------------------------------------------------------------------
// 贴图
// ---------------------------------------------------------------------------

FDoodleOrganizeReport FDoodleAssetOrganizer::ResizeTexturesToPowerOfTwo(const FString& RootPath)
{
	FDoodleOrganizeReport Report;
	Report.Operation = TEXT("ResizeTexturesToPowerOfTwo");

	UEditorAssetSubsystem* EditorAssetSubsystem = GetEditorAssetSubsystem();
	if (EditorAssetSubsystem == nullptr)
	{
		Report.Result = EDoodleOrganizeResult::Failed;
		Report.AddFailure(TEXT("无法获取 UEditorAssetSubsystem"));
		return Report;
	}

	TArray<FAssetData> TextureAssets;
	EnumerateAssets(RootPath, UTexture::StaticClass(), TextureAssets);
	Report.NumRequested = TextureAssets.Num();

	FScopedSlowTask SlowTask(TextureAssets.Num(), LOCTEXT("DoodleResizeTextures", "重置贴图尺寸..."));
	SlowTask.MakeDialog(/*bShowCancelButton*/ true);

	for (const FAssetData& Asset : TextureAssets)
	{
		SlowTask.EnterProgressFrame();
		if (SlowTask.ShouldCancel())
		{
			Report.Result = EDoodleOrganizeResult::Cancelled;
			break;
		}

		UTexture* Texture = Cast<UTexture>(Asset.GetAsset());
		if (Texture == nullptr)
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 无法加载贴图"), *Asset.PackageName.ToString()));
			continue;
		}

		// 原实现在判空之前就取了 In_Texture->Source, 空指针会直接崩
		FTextureSource& Source = Texture->Source;
		if (!Source.IsValid())
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 没有可用的源数据"), *Asset.PackageName.ToString()));
			continue;
		}

		const int32 SizeX = Source.GetSizeX();
		const int32 SizeY = Source.GetSizeY();
		if (SizeX <= 0 || SizeY <= 0)
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 尺寸无效 (%dx%d)"), *Asset.PackageName.ToString(), SizeX, SizeY));
			continue;
		}

		// 2 的幂用位移, 不要用 FMath::Pow 再隐式转 int32
		const int32 MaxSize = 1 << FMath::Max(FMath::CeilLogTwo((uint32)SizeX), FMath::CeilLogTwo((uint32)SizeY));
		if (MaxSize == SizeX && MaxSize == SizeY)
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 已经是 2 的幂 (%dx%d)"), *Asset.PackageName.ToString(), SizeX, SizeY));
			continue;
		}

		FResizeTexture Resize;
		Resize.Resize(Texture);
		EditorAssetSubsystem->SaveLoadedAsset(Texture);

		++Report.NumMoved;
	}

	Report.Finalize();
	return Report;
}

FDoodleOrganizeReport FDoodleAssetOrganizer::PullEngineTexturesReferencedByMaterials(const FString& TargetFolderName)
{
	FDoodleOrganizeReport Report;
	Report.Operation = TEXT("PullEngineTextures");

	const FString FolderName = TargetFolderName.TrimStartAndEnd();
	if (FolderName.IsEmpty())
	{
		Report.Result = EDoodleOrganizeResult::InvalidInput;
		Report.AddFailure(TEXT("目标文件夹名称不能为空"));
		return Report;
	}

	UEditorAssetSubsystem* EditorAssetSubsystem = GetEditorAssetSubsystem();
	IAssetRegistry* Registry = GetAssetRegistry();
	if (EditorAssetSubsystem == nullptr || Registry == nullptr)
	{
		Report.Result = EDoodleOrganizeResult::Failed;
		Report.AddFailure(TEXT("编辑器子系统或资产注册表不可用"));
		return Report;
	}

	const FString GameRoot = DoodleOrganize::GetGameRootPath();
	const FString TargetRoot = DoodleOrganize::CombinePackagePath(GameRoot, FolderName);
	const FString TextureFolder = DoodleOrganize::CombinePackagePath(TargetRoot, TEXT("Tex"));
	const FString BackupFolder = DoodleOrganize::CombinePackagePath(TextureFolder, TEXT("Backup"));

	// 1) 收集 /Game 材质引用的、位于 /Game 之外的贴图包
	TArray<FAssetData> MaterialAssets;
	EnumerateAssets(GameRoot, UMaterialInterface::StaticClass(), MaterialAssets);

	TSet<FName> EngineTexturePackages;
	for (const FAssetData& MaterialAsset : MaterialAssets)
	{
		TArray<FName> Dependencies;
		if (!Registry->GetDependencies(MaterialAsset.PackageName, Dependencies,
			EDependencyCategory::Package, FDependencyQuery(EDependencyQuery::Hard)))
		{
			continue;
		}

		for (const FName& Dependency : Dependencies)
		{
			if (Dependency.ToString().StartsWith(GameRoot, ESearchCase::CaseSensitive))
			{
				continue;
			}
			const FAssetData TextureAsset = FindFirstAssetOfClass(*Registry, Dependency, UTexture::StaticClass());
			if (TextureAsset.IsValid())
			{
				EngineTexturePackages.Add(Dependency);
			}
		}
	}

	Report.NumRequested = EngineTexturePackages.Num();
	EditorAssetSubsystem->MakeDirectory(TextureFolder);

	FScopedSlowTask SlowTask(EngineTexturePackages.Num(), LOCTEXT("DoodlePullEngineTextures", "本地化引擎贴图..."));
	SlowTask.MakeDialog(/*bShowCancelButton*/ true);

	for (const FName& EnginePackage : EngineTexturePackages)
	{
		SlowTask.EnterProgressFrame();
		if (SlowTask.ShouldCancel())
		{
			Report.Result = EDoodleOrganizeResult::Cancelled;
			break;
		}

		const FAssetData SourceAsset = FindFirstAssetOfClass(*Registry, EnginePackage, UTexture::StaticClass());
		if (!SourceAsset.IsValid())
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 无法解析为贴图资产"), *EnginePackage.ToString()));
			continue;
		}

		UObject* SourceObject = SourceAsset.GetAsset();
		if (SourceObject == nullptr)
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 无法加载"), *EnginePackage.ToString()));
			continue;
		}

		const FString BaseName = SourceAsset.AssetName.ToString();
		const FString TargetPackage = DoodleOrganize::CombinePackagePath(TextureFolder, BaseName);

		if (!DoodleOrganize::IsAssetPathFree(TargetPackage))
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 本地已存在 %s, 跳过"), *EnginePackage.ToString(), *TargetPackage));
			continue;
		}

		// 2) 先备份 (原实现把备份写在引擎目录里, 会污染引擎内容; 这里放到工程内的 Backup 目录)
		FString BackupFolderPath;
		FString BackupName;
		if (DoodleOrganize::TryMakeUniqueAssetPath(BackupFolder, BaseName + TEXT("_bu"), BackupFolderPath, BackupName))
		{
			const FString BackupPackage = DoodleOrganize::CombinePackagePath(BackupFolderPath, BackupName);
			UObject* BackupObject = EditorAssetSubsystem->DuplicateAsset(SourceAsset.GetObjectPathString(), BackupPackage);
			if (BackupObject != nullptr)
			{
				EditorAssetSubsystem->SaveLoadedAsset(BackupObject);
			}
			else
			{
				Report.AddWarning(FString::Printf(TEXT("备份 %s 失败, 继续本地化"), *EnginePackage.ToString()));
			}
		}

		// 3) 复制到本地目录
		UObject* LocalObject = EditorAssetSubsystem->DuplicateAsset(SourceAsset.GetObjectPathString(), TargetPackage);
		if (LocalObject == nullptr)
		{
			Report.AddFailure(FString::Printf(TEXT("复制 %s -> %s 失败"), *EnginePackage.ToString(), *TargetPackage));
			continue;
		}

		// 4) 把引用从引擎贴图改指到本地副本
		TArray<UObject*> ObjectsToReplace;
		ObjectsToReplace.Add(SourceObject);
		if (!EditorAssetSubsystem->ConsolidateAssets(LocalObject, ObjectsToReplace))
		{
			Report.AddFailure(FString::Printf(TEXT("改指失败: %s -> %s"), *EnginePackage.ToString(), *TargetPackage));
			continue;
		}

		EditorAssetSubsystem->SaveLoadedAsset(LocalObject);

		++Report.NumMoved;
		Report.MovedPackages.Add(EnginePackage, FName(*TargetPackage));
		Report.AddWarning(FString::Printf(TEXT("已本地化 %s -> %s, 并把引用改指到本地副本"),
			*EnginePackage.ToString(), *TargetPackage));
	}

	Report.Finalize();
	return Report;
}

// ---------------------------------------------------------------------------
// 空目录清理
// ---------------------------------------------------------------------------

FDoodleOrganizeReport FDoodleAssetOrganizer::DeleteEmptyDirectories(const FString& RootPath)
{
	FDoodleOrganizeReport Report;
	Report.Operation = TEXT("DeleteEmptyDirectories");

	IAssetRegistry* Registry = GetAssetRegistry();
	if (Registry == nullptr)
	{
		Report.Result = EDoodleOrganizeResult::Failed;
		Report.AddFailure(TEXT("AssetRegistry 模块未加载"));
		return Report;
	}

	FString RootDirectory;
	if (!FPackageName::TryConvertLongPackageNameToFilename(RootPath, RootDirectory, TEXT("")))
	{
		Report.Result = EDoodleOrganizeResult::InvalidInput;
		Report.AddFailure(FString::Printf(TEXT("无法把 %s 转成磁盘目录"), *RootPath));
		return Report;
	}
	RootDirectory = FPaths::ConvertRelativePathToFull(RootDirectory);

	IFileManager& FileManager = IFileManager::Get();
	if (!FileManager.DirectoryExists(*RootDirectory))
	{
		Report.Result = EDoodleOrganizeResult::InvalidInput;
		Report.AddFailure(FString::Printf(TEXT("目录不存在: %s"), *RootDirectory));
		return Report;
	}

	// 用前缀比较算相对路径, 不用 FPaths::MakePathRelativeTo (那个对尾斜杠的处理容易踩坑)
	FString ContentPrefix = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	FPaths::NormalizeDirectoryName(ContentPrefix);
	ContentPrefix += TEXT("/");

	// 反复扫描直到没有变化: 删掉最深层后父目录才可能变空
	bool bAnyDeleted = true;
	int32 Pass = 0;
	while (bAnyDeleted && Pass < 64)
	{
		++Pass;
		bAnyDeleted = false;

		TArray<FString> Directories;
		FileManager.FindFilesRecursive(Directories, *RootDirectory, TEXT("*"), /*Files*/ false, /*Directories*/ true);

		// 深的先处理
		Directories.Sort([](const FString& A, const FString& B) { return A.Len() > B.Len(); });

		for (const FString& Directory : Directories)
		{
			if (!FileManager.DirectoryExists(*Directory))
			{
				continue;
			}

			// World Partition 的内部目录由引擎管理, 不动
			if (Directory.Contains(TEXT("/__ExternalActors__"), ESearchCase::CaseSensitive)
				|| Directory.Contains(TEXT("/__ExternalObjects__"), ESearchCase::CaseSensitive))
			{
				continue;
			}

			TArray<FString> Entries;
			FileManager.FindFiles(Entries, *FPaths::Combine(Directory, TEXT("*")), /*Files*/ true, /*Directories*/ true);
			if (Entries.Num() > 0)
			{
				continue;
			}

			// 关键修正: 原实现把「文件系统绝对路径」直接塞给 GetAssetsByPath,
			// 那个 API 要的是 /Game/... 包路径, 所以判断永远是"空目录"。
			if (!Directory.StartsWith(ContentPrefix, ESearchCase::IgnoreCase))
			{
				Report.AddSkip(FString::Printf(TEXT("%s : 不在工程 Content 目录下"), *Directory));
				continue;
			}

			const FString RelativeDirectory = Directory.RightChop(ContentPrefix.Len());
			const FString PackagePath = DoodleOrganize::CombinePackagePath(
				DoodleOrganize::GetGameRootPath(), RelativeDirectory);

			TArray<FAssetData> AssetsInPath;
			Registry->GetAssetsByPath(FName(*PackagePath), AssetsInPath, /*bRecursive*/ true);
			if (AssetsInPath.Num() > 0)
			{
				Report.AddSkip(FString::Printf(TEXT("%s : 注册表里仍有 %d 个资产"), *PackagePath, AssetsInPath.Num()));
				continue;
			}

			if (FileManager.DeleteDirectory(*Directory, /*bRequireExists*/ false, /*bTree*/ true))
			{
				++Report.NumMoved;
				bAnyDeleted = true;
			}
			else
			{
				Report.AddFailure(FString::Printf(TEXT("%s : 删除目录失败"), *Directory));
			}
		}
	}

	Report.Finalize();
	return Report;
}

#undef LOCTEXT_NAMESPACE
