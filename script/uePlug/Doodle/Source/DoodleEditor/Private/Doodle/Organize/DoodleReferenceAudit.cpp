// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/DoodleReferenceAudit.h"

#include "Doodle/Organize/DoodleAssetPathUtils.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/DataTable.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Class.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

using namespace UE::AssetRegistry;

namespace
{
	const TCHAR* GSnapshotExtension = TEXT(".json");

	/** 取 (硬引用 + 软引用) 的去重集合, 并剔除 /Script/... 之类的非资产边 */
	TSet<FDoodleReferenceAudit::FEdge> BuildEdgeSet(const FDoodleReferenceAudit::FSnapshot& Snapshot)
	{
		TSet<FDoodleReferenceAudit::FEdge> Result;
		Result.Reserve(Snapshot.HardEdges.Num() + Snapshot.SoftEdges.Num());

		auto AddEdge = [&Result](const FDoodleReferenceAudit::FEdge& Edge)
		{
			// 原生脚本包 (/Script/Engine 之类) 不是资产: 注册表里没有、磁盘上也没有 .uasset,
			// 留着它们只会让「依赖消失」桶里塞满假阳性
			if (DoodleOrganize::IsNonAssetPackagePath(Edge.Dependency.ToString()))
			{
				return;
			}
			Result.Add(Edge);
		};

		for (const FDoodleReferenceAudit::FEdge& Edge : Snapshot.HardEdges)
		{
			AddEdge(Edge);
		}
		for (const FDoodleReferenceAudit::FEdge& Edge : Snapshot.SoftEdges)
		{
			AddEdge(Edge);
		}
		return Result;
	}

	bool DependencyStillExists(const FName Dependency, const FDoodleReferenceAudit::FSnapshot& After,
		const FDoodleReferenceAudit::FDiffOptions& Options)
	{
		if (Options.bCheckRegistryForExistence)
		{
			const FString PackageName = Dependency.ToString();
			return DoodleOrganize::DoesAssetExistOnDisk(PackageName)
				|| DoodleOrganize::DoesAssetExistInRegistry(PackageName, /*bIncludeRedirectors*/ true);
		}
		return After.Packages.Contains(Dependency);
	}

	void AppendEntry(FString& Out, const TCHAR* Title, const TArray<FDoodleReferenceAudit::FDiffEntry>& Entries, int32 MaxEntries)
	{
		if (Entries.Num() == 0)
		{
			return;
		}

		Out += FString::Printf(TEXT("\n%s (%d):\n"), Title, Entries.Num());
		const int32 Count = FMath::Min(Entries.Num(), MaxEntries);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FDoodleReferenceAudit::FDiffEntry& Entry = Entries[Index];
			if (Entry.RetargetedTo.IsNone())
			{
				Out += FString::Printf(TEXT("  %s -> %s%s\n"),
					*Entry.Package.ToString(), *Entry.Dependency.ToString(),
					Entry.Note.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("   (%s)"), *Entry.Note));
			}
			else
			{
				Out += FString::Printf(TEXT("  %s -> %s  ==> %s%s\n"),
					*Entry.Package.ToString(), *Entry.Dependency.ToString(), *Entry.RetargetedTo.ToString(),
					Entry.Note.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("   (%s)"), *Entry.Note));
			}
		}
		if (Entries.Num() > Count)
		{
			Out += FString::Printf(TEXT("  ... 还有 %d 条, 见 CSV 报告\n"), Entries.Num() - Count);
		}
	}
}

// ---------------------------------------------------------------------------
// 快照读写
// ---------------------------------------------------------------------------

bool FDoodleReferenceAudit::FSnapshot::Save(const FString& Filename) const
{
	FString Output;
	{
		const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Output);

		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("version"), 1);
		Writer->WriteValue(TEXT("takenAt"), TakenAt.ToIso8601());
		Writer->WriteValue(TEXT("root"), RootPath);

		Writer->WriteArrayStart(TEXT("packages"));
		for (const FName& Package : Packages)
		{
			Writer->WriteValue(Package.ToString());
		}
		Writer->WriteArrayEnd();

		const auto WriteEdges = [&Writer](const TCHAR* FieldName, const TArray<FEdge>& Edges)
		{
			Writer->WriteArrayStart(FieldName);
			for (const FEdge& Edge : Edges)
			{
				Writer->WriteObjectStart();
				Writer->WriteValue(TEXT("p"), Edge.Package.ToString());
				Writer->WriteValue(TEXT("d"), Edge.Dependency.ToString());
				Writer->WriteObjectEnd();
			}
			Writer->WriteArrayEnd();
		};

		WriteEdges(TEXT("hardEdges"), HardEdges);
		WriteEdges(TEXT("softEdges"), SoftEdges);

		Writer->WriteObjectEnd();
		Writer->Close();
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString Directory = FPaths::GetPath(Filename);
	if (!Directory.IsEmpty() && !PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	return FFileHelper::SaveStringToFile(Output, *Filename);
}

bool FDoodleReferenceAudit::FSnapshot::Load(const FString& Filename, FSnapshot& Out)
{
	FString Input;
	if (!FFileHelper::LoadFileToString(Input, *Filename))
	{
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Input);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return false;
	}

	Out = FSnapshot();

	FString TakenAtString;
	if (RootObject->TryGetStringField(TEXT("takenAt"), TakenAtString))
	{
		FDateTime::ParseIso8601(*TakenAtString, Out.TakenAt);
	}
	RootObject->TryGetStringField(TEXT("root"), Out.RootPath);

	const TArray<TSharedPtr<FJsonValue>>* PackagesArray = nullptr;
	if (RootObject->TryGetArrayField(TEXT("packages"), PackagesArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *PackagesArray)
		{
			FString PackageString;
			if (Value.IsValid() && Value->TryGetString(PackageString))
			{
				Out.Packages.Add(FName(*PackageString));
			}
		}
	}

	const auto ReadEdges = [&RootObject](const TCHAR* FieldName, TArray<FEdge>& Edges)
	{
		const TArray<TSharedPtr<FJsonValue>>* EdgeArray = nullptr;
		if (!RootObject->TryGetArrayField(FieldName, EdgeArray))
		{
			return;
		}
		for (const TSharedPtr<FJsonValue>& Value : *EdgeArray)
		{
			const TSharedPtr<FJsonObject>* EdgeObject = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(EdgeObject) || EdgeObject == nullptr)
			{
				continue;
			}
			FString PackageString;
			FString DependencyString;
			(*EdgeObject)->TryGetStringField(TEXT("p"), PackageString);
			(*EdgeObject)->TryGetStringField(TEXT("d"), DependencyString);
			if (!PackageString.IsEmpty() && !DependencyString.IsEmpty())
			{
				Edges.Add({ FName(*PackageString), FName(*DependencyString) });
			}
		}
	};

	ReadEdges(TEXT("hardEdges"), Out.HardEdges);
	ReadEdges(TEXT("softEdges"), Out.SoftEdges);
	return true;
}

// ---------------------------------------------------------------------------
// 采集
// ---------------------------------------------------------------------------

bool FDoodleReferenceAudit::Capture(const FString& RootPath, FSnapshot& Out)
{
	Out = FSnapshot();
	Out.TakenAt = FDateTime::UtcNow();
	Out.RootPath = RootPath;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName(*RootPath));

	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);

	Out.Packages.Reserve(Assets.Num());
	for (const FAssetData& Asset : Assets)
	{
		Out.Packages.Add(Asset.PackageName);
	}

	for (const FName& PackageName : Out.Packages)
	{
		TArray<FName> HardDependencies;
		if (Registry.GetDependencies(PackageName, HardDependencies, EDependencyCategory::Package, FDependencyQuery(EDependencyQuery::Hard)))
		{
			for (const FName& Dependency : HardDependencies)
			{
				if (Dependency != PackageName)
				{
					Out.HardEdges.Add({ PackageName, Dependency });
				}
			}
		}

		TArray<FName> SoftDependencies;
		if (Registry.GetDependencies(PackageName, SoftDependencies, EDependencyCategory::Package, FDependencyQuery(EDependencyQuery::Soft)))
		{
			for (const FName& Dependency : SoftDependencies)
			{
				if (Dependency != PackageName)
				{
					Out.SoftEdges.Add({ PackageName, Dependency });
				}
			}
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// 差异
// ---------------------------------------------------------------------------

FDoodleReferenceAudit::FDiffResult FDoodleReferenceAudit::Diff(
	const FSnapshot& Before, const FSnapshot& After, const FDiffOptions& Options)
{
	FDiffResult Result;
	Result.NumEdgesBefore = Before.HardEdges.Num() + Before.SoftEdges.Num();
	Result.NumEdgesAfter = After.HardEdges.Num() + After.SoftEdges.Num();
	Result.NumPackagesBefore = Before.Packages.Num();
	Result.NumPackagesAfter = After.Packages.Num();

	const TSet<FEdge> BeforeEdges = BuildEdgeSet(Before);
	const TSet<FEdge> AfterEdges = BuildEdgeSet(After);

	// "正确改指"的边: 它们相对 Before 也是新出现的, 但语义是"被修好了", 不该再算进"新增边"
	TSet<FEdge> RetargetedEdgeSet;

	// 消失的边
	for (const FEdge& Edge : BeforeEdges)
	{
		if (AfterEdges.Contains(Edge))
		{
			continue;
		}

		FDiffEntry Entry;
		Entry.Package = Edge.Package;
		Entry.Dependency = Edge.Dependency;

		const FName* RetargetedTo = Options.MoveMap.Find(Edge.Dependency);
		if (RetargetedTo != nullptr && !RetargetedTo->IsNone())
		{
			Entry.RetargetedTo = *RetargetedTo;
			if (AfterEdges.Contains({ Edge.Package, *RetargetedTo }))
			{
				// 依赖被搬走了, 引用也正确跟着改了
				Result.RetargetedEdges.Add(Entry);
				RetargetedEdgeSet.Add({ Edge.Package, *RetargetedTo });
			}
			else
			{
				// 依赖被搬走了, 但引用没有跟着改 —— 这就是要找的真·引用丢失
				Entry.Note = FString::Printf(TEXT("依赖已移动到 %s, 但该引用没有被修复"), *RetargetedTo->ToString());
				Result.LostEdges.Add(Entry);
			}
			continue;
		}

		if (DependencyStillExists(Edge.Dependency, After, Options))
		{
			Entry.Note = TEXT("依赖仍然存在, 但引用边消失了");
			Result.LostEdges.Add(Entry);
		}
		else
		{
			Entry.Note = TEXT("依赖包本身也不存在了 (可能在本次操作之外被删除/移动)");
			Result.LostDependencies.Add(Entry);
		}
	}

	// 新增的边 (正确改指的不算"新增", 否则同一条边会在两个桶里各出现一次)
	for (const FEdge& Edge : AfterEdges)
	{
		if (!BeforeEdges.Contains(Edge) && !RetargetedEdgeSet.Contains(Edge))
		{
			Result.AddedEdges.Add({ Edge.Package, Edge.Dependency, NAME_None, FString() });
		}
	}

	return Result;
}

TArray<FDoodleReferenceAudit::FDiffEntry> FDoodleReferenceAudit::FindDanglingReferences(const FString& RootPath)
{
	TArray<FDiffEntry> Result;

	FSnapshot Snapshot;
	if (!Capture(RootPath, Snapshot))
	{
		return Result;
	}

	const TSet<FEdge> Edges = BuildEdgeSet(Snapshot);
	for (const FEdge& Edge : Edges)
	{
		const FString DependencyName = Edge.Dependency.ToString();

		// /Script/... 之类的原生包永远不在注册表里也没有 .uasset, 不排除会造成大量假阳性
		if (DoodleOrganize::IsNonAssetPackagePath(DependencyName))
		{
			continue;
		}

		if (!DoodleOrganize::DoesAssetExistOnDisk(DependencyName)
			&& !DoodleOrganize::DoesAssetExistInRegistry(DependencyName, /*bIncludeRedirectors*/ true))
		{
			Result.Add({ Edge.Package, Edge.Dependency, NAME_None, TEXT("依赖包在磁盘与注册表中都不存在") });
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------
// 报告输出
// ---------------------------------------------------------------------------

FString FDoodleReferenceAudit::FDiffResult::ToText(int32 MaxEntries) const
{
	FString Out = FString::Printf(
		TEXT("引用差异报告\n  边: %d -> %d    包: %d -> %d\n  真·引用丢失: %d    依赖消失: %d    正确改指: %d    新增边: %d\n"),
		NumEdgesBefore, NumEdgesAfter, NumPackagesBefore, NumPackagesAfter,
		LostEdges.Num(), LostDependencies.Num(), RetargetedEdges.Num(), AddedEdges.Num());

	if (HasRealLoss())
	{
		Out += TEXT("\n*** 检测到真·引用丢失 ***\n");
	}

	AppendEntry(Out, TEXT("真·引用丢失 (依赖仍在, 引用没跟上)"), LostEdges, MaxEntries);
	AppendEntry(Out, TEXT("依赖包消失 (需人工确认)"), LostDependencies, MaxEntries);
	AppendEntry(Out, TEXT("正确改指"), RetargetedEdges, MaxEntries);
	AppendEntry(Out, TEXT("新增边"), AddedEdges, MaxEntries);

	return Out;
}

FString FDoodleReferenceAudit::FDiffResult::ToCsv() const
{
	FString Out = TEXT("kind,package,dependency,retargetedTo,note\n");

	const auto AppendRows = [&Out](const TCHAR* Kind, const TArray<FDiffEntry>& Entries)
	{
		for (const FDiffEntry& Entry : Entries)
		{
			Out += FString::Printf(TEXT("%s,%s,%s,%s,\"%s\"\n"),
				Kind,
				*Entry.Package.ToString(),
				*Entry.Dependency.ToString(),
				Entry.RetargetedTo.IsNone() ? TEXT("") : *Entry.RetargetedTo.ToString(),
				*Entry.Note.Replace(TEXT("\""), TEXT("'")));
		}
	};

	AppendRows(TEXT("LostEdge"), LostEdges);
	AppendRows(TEXT("LostDependency"), LostDependencies);
	AppendRows(TEXT("Retargeted"), RetargetedEdges);
	AppendRows(TEXT("Added"), AddedEdges);

	return Out;
}

FString FDoodleReferenceAudit::GetReportDirectory()
{
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("DoodleOrganize");

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	return Directory;
}

FString FDoodleReferenceAudit::MakeSnapshotFilename(const TCHAR* Tag)
{
	const FString TimeStamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	return GetReportDirectory() / FString::Printf(TEXT("refs_%s_%s%s"), Tag, *TimeStamp, GSnapshotExtension);
}

FString FDoodleReferenceAudit::WriteReportFile(const FString& BaseName, const FString& Content)
{
	const FString Directory = GetReportDirectory();
	const FString TimeStamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString Filename = Directory / FString::Printf(TEXT("%s_%s.txt"), *BaseName, *TimeStamp);

	if (!FFileHelper::SaveStringToFile(Content, *Filename))
	{
		return FString();
	}
	return Filename;
}

// ---------------------------------------------------------------------------
// 内存软引用扫描 (注册表盲区)
// ---------------------------------------------------------------------------

namespace
{
	using FDiffEntry = FDoodleReferenceAudit::FDiffEntry;

	struct FSoftScanContext
	{
		FString RootPath;
		bool bDirtyPackagesOnly = true;
		int32 MaxEntries = 2000;

		// 只读模式 (查悬空引用)
		TArray<FDiffEntry>* Out = nullptr;
		TSet<FString>* Seen = nullptr;

		// 修复模式 (改软引用): 旧包名 -> 新包名
		const TMap<FString, FString>* RetargetMap = nullptr;
		TSet<FString>* TouchedPackages = nullptr;
		FString CurrentPackageName;
		bool bChangedCurrentPackage = false;
		int32 NumRetargeted = 0;
		/** 遍历器看到的软引用条数 (不管悬空与否), 用来确认遍历真的覆盖到了东西 */
		int32 NumSeen = 0;
		/** 命中的软引用属性个数 (含空值), 用来区分「没有软引用属性」和「属性值为空」 */
		int32 NumSoftProperties = 0;
		/** 覆盖统计 (只读模式用) */
		FDoodleReferenceAudit::FSoftScanStats Stats;
		TSet<FString> VisitedPackages;
	};

	/**
	 * 处理一条软引用。
	 * 修复模式: 命中 MoveMap 就改写, 返回 true。
	 * 只读模式: 目标既不在磁盘也不在注册表就登记为悬空, 返回 false。
	 */
	bool HandleSoftPath(const FString& ReferencerObjectPath, FSoftObjectPath& Path,
		bool bDirtyPackage, FSoftScanContext& Context)
	{
		const FString TargetPath = Path.ToString();
		if (TargetPath.IsEmpty())
		{
			return false;
		}

		const FString PackageName = DoodleOrganize::StripObjectPath(TargetPath);
		if (!PackageName.StartsWith(Context.RootPath, ESearchCase::CaseSensitive))
		{
			return false;
		}

		++Context.NumSeen;

		if (Context.RetargetMap != nullptr)
		{
			// 纯函数, 单测覆盖: Doodle.Organize.PathUtils.Retarget
			const FString NewTarget = DoodleOrganize::RetargetObjectPath(TargetPath, *Context.RetargetMap);
			if (NewTarget.IsEmpty())
			{
				return false;
			}

			Path = FSoftObjectPath(NewTarget);

			++Context.NumRetargeted;
			Context.bChangedCurrentPackage = true;
			if (Context.TouchedPackages != nullptr && !Context.CurrentPackageName.IsEmpty())
			{
				Context.TouchedPackages->Add(Context.CurrentPackageName);
			}
			return true;
		}

		if (Context.Out == nullptr || Context.Out->Num() >= Context.MaxEntries)
		{
			return false;
		}

		// /Script/... 之类的原生包不是资产, 跳过 (否则会报出大量假阳性)
		if (DoodleOrganize::IsNonAssetPackagePath(PackageName))
		{
			return false;
		}

		// 目标还在磁盘或注册表里 => 没丢
		if (DoodleOrganize::DoesAssetExistOnDisk(PackageName)
			|| DoodleOrganize::DoesAssetExistInRegistry(PackageName, /*bIncludeRedirectors*/ true))
		{
			return false;
		}

		const FString Key = ReferencerObjectPath + TEXT("|") + TargetPath;
		if (Context.Seen->Contains(Key))
		{
			return false;
		}
		Context.Seen->Add(Key);

		FDiffEntry Entry;
		Entry.Package = FName(*ReferencerObjectPath);
		Entry.Dependency = FName(*TargetPath);
		Entry.Note = bDirtyPackage
			? TEXT("悬空软引用, 且所在包尚未保存 (注册表看不见, 引擎的自动修复不会覆盖)")
			: TEXT("悬空软引用");
		Context.Out->Add(MoveTemp(Entry));
		return false;
	}

	void CollectSoftPathsFromProperty(const FProperty* Property, void* ValuePtr,
		const FString& ReferencerObjectPath, bool bDirtyPackage, int32 Depth, FSoftScanContext& Context);

	void CollectSoftPathsFromStruct(const UScriptStruct* Struct, void* ValuePtr,
		const FString& ReferencerObjectPath, bool bDirtyPackage, int32 Depth, FSoftScanContext& Context)
	{
		if (Struct == nullptr || ValuePtr == nullptr)
		{
			return;
		}

		// FSoftObjectPath / FSoftClassPath (FSoftClassPath 派生自 FSoftObjectPath 且没有额外成员)
		const UScriptStruct* SoftPathStruct = TBaseStructure<FSoftObjectPath>::Get();
		if (SoftPathStruct != nullptr && Struct->IsChildOf(SoftPathStruct))
		{
			++Context.NumSoftProperties;
			FSoftObjectPath& Path = *static_cast<FSoftObjectPath*>(ValuePtr);
			HandleSoftPath(ReferencerObjectPath, Path, bDirtyPackage, Context);
			return;
		}

		// 自定义结构体里可能嵌了软引用 (深度受限, 避免在大结构上浪费时间)
		if (Depth >= 3)
		{
			return;
		}

		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			const FProperty* Inner = *It;
			void* InnerValue = Inner->ContainerPtrToValuePtr<void>(ValuePtr);
			CollectSoftPathsFromProperty(Inner, InnerValue, ReferencerObjectPath, bDirtyPackage, Depth + 1, Context);
		}
	}

	void CollectSoftPathsFromProperty(const FProperty* Property, void* ValuePtr,
		const FString& ReferencerObjectPath, bool bDirtyPackage, int32 Depth, FSoftScanContext& Context)
	{
		if (Property == nullptr || ValuePtr == nullptr)
		{
			return;
		}

		// TSoftObjectPtr<T> / TSoftClassPtr<T>
		if (const FSoftObjectProperty* SoftProperty = CastField<FSoftObjectProperty>(Property))
		{
			++Context.NumSoftProperties;

			// 注意: ValuePtr 已经是「属性值的地址」(调用方用 ContainerPtrToValuePtr 算好了)。
			// 这里只能用 GetPropertyValuePtr (纯转换), 绝对不能用 GetPropertyValuePtr_InContainer ——
			// 后者把入参当容器再偏移一次属性 offset, 得到垃圾指针, 实测会直接崩编辑器
			// (EXCEPTION_ACCESS_VIOLATION in FSoftObjectPath::FSoftObjectPath)。
			FSoftObjectPtr* SoftValue = SoftProperty->GetPropertyValuePtr(ValuePtr);

			FSoftObjectPath Path = SoftValue->ToSoftObjectPath();
			if (HandleSoftPath(ReferencerObjectPath, Path, bDirtyPackage, Context))
			{
				*SoftValue = FSoftObjectPtr(Path);
			}
			return;
		}

		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			CollectSoftPathsFromStruct(StructProperty->Struct, ValuePtr, ReferencerObjectPath, bDirtyPackage, Depth, Context);
			return;
		}

		if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, ValuePtr);
			for (int32 Index = 0; Index < Helper.Num(); ++Index)
			{
				CollectSoftPathsFromProperty(ArrayProperty->Inner, Helper.GetRawPtr(Index),
					ReferencerObjectPath, bDirtyPackage, Depth + 1, Context);
			}
			return;
		}

		// TMap<K, TSoftObjectPtr<T>> —— 键和值都要看 (键也可能是软引用)
		if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
		{
			FScriptMapHelper Helper(MapProperty, ValuePtr);
			const int32 MaxIndex = Helper.GetMaxIndex();
			for (int32 Index = 0; Index < MaxIndex; ++Index)
			{
				if (!Helper.IsValidIndex(Index))
				{
					continue;
				}
				CollectSoftPathsFromProperty(MapProperty->KeyProp, Helper.GetKeyPtr(Index),
					ReferencerObjectPath, bDirtyPackage, Depth + 1, Context);
				CollectSoftPathsFromProperty(MapProperty->ValueProp, Helper.GetValuePtr(Index),
					ReferencerObjectPath, bDirtyPackage, Depth + 1, Context);
			}
			return;
		}

		// TSet<TSoftObjectPtr<T>>
		if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
		{
			FScriptSetHelper Helper(SetProperty, ValuePtr);
			const int32 MaxIndex = Helper.GetMaxIndex();
			for (int32 Index = 0; Index < MaxIndex; ++Index)
			{
				if (!Helper.IsValidIndex(Index))
				{
					continue;
				}
				CollectSoftPathsFromProperty(SetProperty->ElementProp, Helper.GetElementPtr(Index),
					ReferencerObjectPath, bDirtyPackage, Depth + 1, Context);
			}
			return;
		}
	}

	/** 把 DataTable 的行数据也过一遍 (行数据不在普通属性遍历里) */
	void CollectSoftPathsFromDataTable(UDataTable* DataTable, const FString& ReferencerObjectPath,
		bool bDirtyPackage, FSoftScanContext& Context)
	{
		if (DataTable == nullptr)
		{
			return;
		}

		const UScriptStruct* RowStruct = DataTable->GetRowStruct();
		if (RowStruct == nullptr)
		{
			return;
		}

		for (const TPair<FName, uint8*>& Row : DataTable->GetRowMap())
		{
			if (Row.Value == nullptr)
			{
				continue;
			}

			const FString RowPath = FString::Printf(TEXT("%s:%s"), *ReferencerObjectPath, *Row.Key.ToString());
			for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
			{
				const FProperty* Inner = *It;
				CollectSoftPathsFromProperty(Inner, Inner->ContainerPtrToValuePtr<void>(Row.Value),
					RowPath, bDirtyPackage, 1, Context);
			}
		}
	}

	/** 遍历 RootPath 下已加载对象的所有软引用 (含 DataTable 行) */
	void WalkSoftReferences(FSoftScanContext& Context)
	{
		if (Context.RootPath.IsEmpty())
		{
			return;
		}

		for (TObjectIterator<UObject> It; It; ++It)
		{
			UObject* Object = *It;
			if (Object == nullptr || Object->IsA<UPackage>() || Object->IsA<UObjectRedirector>())
			{
				continue;
			}

			UPackage* Package = Object->GetOutermost();
			if (Package == nullptr || Package->HasAnyPackageFlags((uint32)(PKG_CompiledIn | PKG_PlayInEditor)))
			{
				continue;
			}

			const FString PackageName = Package->GetName();
			if (!PackageName.StartsWith(Context.RootPath, ESearchCase::CaseSensitive))
			{
				continue;
			}

			// 脏包 / 从未落盘的包 = 注册表覆盖不到的盲区
			const bool bDirtyPackage = Package->IsDirty() || !FPackageName::DoesPackageExist(PackageName);
			if (Context.bDirtyPackagesOnly && !bDirtyPackage)
			{
				continue;
			}

			++Context.Stats.NumObjectsVisited;
			Context.VisitedPackages.Add(PackageName);

			Context.CurrentPackageName = PackageName;
			Context.bChangedCurrentPackage = false;

			const FString ObjectPath = Object->GetPathName();

			if (UDataTable* DataTable = Cast<UDataTable>(Object))
			{
				CollectSoftPathsFromDataTable(DataTable, ObjectPath, bDirtyPackage, Context);
			}

			for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
			{
				const FProperty* Property = *PropIt;
				CollectSoftPathsFromProperty(Property, Property->ContainerPtrToValuePtr<void>(Object),
					ObjectPath, bDirtyPackage, 0, Context);
			}

			// 改写过 => 必须把包标脏, 否则改动不会落盘
			if (Context.bChangedCurrentPackage)
			{
				Package->MarkPackageDirty();
			}

			if (Context.Out != nullptr && Context.Out->Num() >= Context.MaxEntries)
			{
				break;
			}
		}
	}
}

TArray<FDoodleReferenceAudit::FDiffEntry> FDoodleReferenceAudit::FindDanglingSoftReferences(
	const FString& RootPath, bool bDirtyPackagesOnly, int32 MaxEntries)
{
	TArray<FDiffEntry> Result;
	TSet<FString> Seen;

	FSoftScanContext Context;
	Context.RootPath = RootPath;
	Context.bDirtyPackagesOnly = bDirtyPackagesOnly;
	Context.MaxEntries = FMath::Max(1, MaxEntries);
	Context.Out = &Result;
	Context.Seen = &Seen;

	WalkSoftReferences(Context);

	Result.Sort([](const FDiffEntry& A, const FDiffEntry& B)
	{
		return A.Package.LexicalLess(B.Package);
	});

	return Result;
}

FDoodleReferenceAudit::FSoftScanStats FDoodleReferenceAudit::ScanSoftReferences(
	const FString& RootPath, bool bDirtyPackagesOnly)
{
	FSoftScanContext Context;
	Context.RootPath = RootPath;
	Context.bDirtyPackagesOnly = bDirtyPackagesOnly;
	Context.MaxEntries = MAX_int32;

	WalkSoftReferences(Context);

	Context.Stats.NumPackagesVisited = Context.VisitedPackages.Num();
	Context.Stats.NumSoftPropertiesSeen = Context.NumSoftProperties;
	Context.Stats.NumSoftPathsSeen = Context.NumSeen;

	return Context.Stats;
}

int32 FDoodleReferenceAudit::RetargetSoftReferences(const TMap<FName, FName>& MoveMap,
	const FString& RootPath, TArray<FString>& OutTouchedPackages)
{
	OutTouchedPackages.Reset();

	// MoveMap 的键值可能是对象路径, 这里统一按包名比较
	TMap<FString, FString> RetargetMap;
	for (const TPair<FName, FName>& Pair : MoveMap)
	{
		const FString OldPackage = DoodleOrganize::StripObjectPath(Pair.Key.ToString());
		const FString NewPackage = DoodleOrganize::StripObjectPath(Pair.Value.ToString());
		if (!OldPackage.IsEmpty() && !NewPackage.IsEmpty() && OldPackage != NewPackage)
		{
			RetargetMap.Add(OldPackage, NewPackage);
		}
	}

	if (RetargetMap.Num() == 0)
	{
		return 0;
	}

	TSet<FString> Touched;

	FSoftScanContext Context;
	Context.RootPath = RootPath;
	// 引用者可能已经落盘, 也要一起修 —— 只读模式下这里会漏掉它们
	Context.bDirtyPackagesOnly = false;
	Context.MaxEntries = MAX_int32;
	Context.RetargetMap = &RetargetMap;
	Context.TouchedPackages = &Touched;

	WalkSoftReferences(Context);

	OutTouchedPackages = Touched.Array();
	OutTouchedPackages.Sort();

	return Context.NumRetargeted;
}

// ---------------------------------------------------------------------------
// 控制台命令 (免 UI 取证)
// ---------------------------------------------------------------------------

namespace
{
	void DoodleDumpReferencesCommand(const TArray<FString>& Args)
	{
		const FString RootPath = Args.Num() > 0 ? Args[0] : TEXT("/Game");

		FDoodleReferenceAudit::FSnapshot Snapshot;
		if (!FDoodleReferenceAudit::Capture(RootPath, Snapshot))
		{
			UE_LOG(LogTemp, Error, TEXT("[DoodleOrganize] 采集引用快照失败: %s"), *RootPath);
			return;
		}

		const FString Filename = FDoodleReferenceAudit::MakeSnapshotFilename(TEXT("dump"));
		if (!Snapshot.Save(Filename))
		{
			UE_LOG(LogTemp, Error, TEXT("[DoodleOrganize] 写入快照失败: %s"), *Filename);
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("[DoodleOrganize] 快照已写入 %s  (包 %d / 硬边 %d / 软边 %d)"),
			*Filename, Snapshot.Packages.Num(), Snapshot.HardEdges.Num(), Snapshot.SoftEdges.Num());
	}

	void DoodleCompareSnapshotsCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogTemp, Error, TEXT("用法: Doodle.Organize.CompareSnapshots <Before.json> <After.json>"));
			return;
		}

		FDoodleReferenceAudit::FSnapshot Before;
		FDoodleReferenceAudit::FSnapshot After;
		if (!FDoodleReferenceAudit::FSnapshot::Load(Args[0], Before))
		{
			UE_LOG(LogTemp, Error, TEXT("[DoodleOrganize] 无法读取快照: %s"), *Args[0]);
			return;
		}
		if (!FDoodleReferenceAudit::FSnapshot::Load(Args[1], After))
		{
			UE_LOG(LogTemp, Error, TEXT("[DoodleOrganize] 无法读取快照: %s"), *Args[1]);
			return;
		}

		const FDoodleReferenceAudit::FDiffResult DiffResult = FDoodleReferenceAudit::Diff(Before, After);
		UE_LOG(LogTemp, Log, TEXT("[DoodleOrganize] %s"), *DiffResult.ToText(50));

		const FString Filename = FDoodleReferenceAudit::WriteReportFile(TEXT("refdiff"), DiffResult.ToText(100000));
		const FString CsvFilename = FDoodleReferenceAudit::WriteReportFile(TEXT("refdiff"), DiffResult.ToCsv());
		UE_LOG(LogTemp, Log, TEXT("[DoodleOrganize] 报告: %s   CSV: %s"), *Filename, *CsvFilename);
	}

	void DoodleFindDanglingCommand(const TArray<FString>& Args)
	{
		const FString RootPath = Args.Num() > 0 ? Args[0] : TEXT("/Game");

		const TArray<FDoodleReferenceAudit::FDiffEntry> Dangling = FDoodleReferenceAudit::FindDanglingReferences(RootPath);
		UE_LOG(LogTemp, Log, TEXT("[DoodleOrganize] 悬空引用 %d 条"), Dangling.Num());
		for (int32 Index = 0; Index < FMath::Min(Dangling.Num(), 200); ++Index)
		{
			UE_LOG(LogTemp, Log, TEXT("  %s -> %s"), *Dangling[Index].Package.ToString(), *Dangling[Index].Dependency.ToString());
		}
	}

	void DoodleFindDanglingSoftRefsCommand(const TArray<FString>& Args)
	{
		const FString RootPath = Args.Num() > 0 ? Args[0] : TEXT("/Game");

		// 参数 1: all = 连已保存的包一起查; 默认只查脏包
		const bool bDirtyOnly = !(Args.Num() > 1 && Args[1].Equals(TEXT("all"), ESearchCase::IgnoreCase));

		const TArray<FDoodleReferenceAudit::FDiffEntry> Dangling =
			FDoodleReferenceAudit::FindDanglingSoftReferences(RootPath, bDirtyOnly);

		UE_LOG(LogTemp, Log, TEXT("[DoodleOrganize] 悬空软引用 %d 条 (范围: %s)"),
			Dangling.Num(), bDirtyOnly ? TEXT("仅未保存的包") : TEXT("全部已加载包"));

		for (int32 Index = 0; Index < FMath::Min(Dangling.Num(), 200); ++Index)
		{
			UE_LOG(LogTemp, Log, TEXT("  %s -> %s  (%s)"),
				*Dangling[Index].Package.ToString(), *Dangling[Index].Dependency.ToString(), *Dangling[Index].Note);
		}

		if (Dangling.Num() > 0)
		{
			FString Report;
			Report += FString::Printf(TEXT("悬空软引用 %d 条\n\n"), Dangling.Num());
			for (const FDoodleReferenceAudit::FDiffEntry& Entry : Dangling)
			{
				Report += FString::Printf(TEXT("%s -> %s\n    %s\n"),
					*Entry.Package.ToString(), *Entry.Dependency.ToString(), *Entry.Note);
			}
			const FString Filename = FDoodleReferenceAudit::WriteReportFile(TEXT("dangling_softrefs"), Report);
			UE_LOG(LogTemp, Log, TEXT("[DoodleOrganize] 报告: %s"), *Filename);
		}
	}

	void DoodleRetargetSoftRefsCommand(const TArray<FString>& Args)
	{
		// 用法: Doodle.Organize.RetargetSoftRefs <RootPath> <旧包> <新包> [<旧包2> <新包2> ...]
		if (Args.Num() < 3 || (Args.Num() % 2) == 0)
		{
			UE_LOG(LogTemp, Error,
				TEXT("用法: Doodle.Organize.RetargetSoftRefs <RootPath> <旧包> <新包> [<旧包2> <新包2> ...]"));
			return;
		}

		const FString RootPath = Args[0];

		TMap<FName, FName> MoveMap;
		for (int32 Index = 1; Index + 1 < Args.Num(); Index += 2)
		{
			MoveMap.Add(FName(*Args[Index]), FName(*Args[Index + 1]));
		}

		TArray<FString> TouchedPackages;
		const int32 NumRetargeted = FDoodleReferenceAudit::RetargetSoftReferences(
			MoveMap, RootPath, TouchedPackages);

		UE_LOG(LogTemp, Log, TEXT("[DoodleOrganize] 补修软引用 %d 条, 涉及 %d 个包 (需要保存)"),
			NumRetargeted, TouchedPackages.Num());
		for (const FString& PackageName : TouchedPackages)
		{
			UE_LOG(LogTemp, Log, TEXT("  待保存: %s"), *PackageName);
		}

		if (TouchedPackages.Num() > 0)
		{
			FString Report = FString::Printf(TEXT("补修软引用 %d 条\n\n待保存的包:\n"), NumRetargeted);
			for (const FString& PackageName : TouchedPackages)
			{
				Report += FString::Printf(TEXT("%s\n"), *PackageName);
			}
			const FString Filename = FDoodleReferenceAudit::WriteReportFile(TEXT("retarget_softrefs"), Report);
			UE_LOG(LogTemp, Log, TEXT("[DoodleOrganize] 报告: %s"), *Filename);
		}
	}

	static FAutoConsoleCommand GDoodleDumpReferencesCommand(
		TEXT("Doodle.Organize.DumpReferences"),
		TEXT("导出 /Game 下的引用边快照到 Saved/DoodleOrganize/refs_dump_<时间戳>.json"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DoodleDumpReferencesCommand));

	static FAutoConsoleCommand GDoodleCompareSnapshotsCommand(
		TEXT("Doodle.Organize.CompareSnapshots"),
		TEXT("比较两个引用快照: Doodle.Organize.CompareSnapshots <Before.json> <After.json>"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DoodleCompareSnapshotsCommand));

	static FAutoConsoleCommand GDoodleFindDanglingCommand(
		TEXT("Doodle.Organize.FindDangling"),
		TEXT("扫描 /Game 下依赖包已不存在的悬空引用"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DoodleFindDanglingCommand));

	static FAutoConsoleCommand GDoodleFindDanglingSoftRefsCommand(
		TEXT("Doodle.Organize.FindDanglingSoftRefs"),
		TEXT("扫描已加载对象里指向已不存在路径的软引用 (默认只查未保存的包, 加 all 查全部): Doodle.Organize.FindDanglingSoftRefs [/Game] [all]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DoodleFindDanglingSoftRefsCommand));

	static FAutoConsoleCommand GDoodleRetargetSoftRefsCommand(
		TEXT("Doodle.Organize.RetargetSoftRefs"),
		TEXT("补修软引用 (修复根因/救回已丢的引用): Doodle.Organize.RetargetSoftRefs <RootPath> <旧包> <新包> [<旧包2> <新包2> ...]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DoodleRetargetSoftRefsCommand));
}
