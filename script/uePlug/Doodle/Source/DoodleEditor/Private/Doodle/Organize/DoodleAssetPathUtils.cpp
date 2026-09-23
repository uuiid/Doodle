// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/DoodleAssetPathUtils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/ObjectRedirector.h"

namespace DoodleOrganize
{
	FString StripObjectPath(const FString& PackageOrObjectPath)
	{
		FString Result = PackageOrObjectPath;
		int32 ColonIndex = INDEX_NONE;
		if (Result.FindChar(TEXT(':'), ColonIndex))
		{
			Result = Result.Left(ColonIndex);
		}
		int32 DotIndex = INDEX_NONE;
		if (Result.FindChar(TEXT('.'), DotIndex))
		{
			Result = Result.Left(DotIndex);
		}
		return Result;
	}

	FString CombinePackagePath(const FString& InFolder, const FString& InName)
	{
		FString Folder = InFolder;
		Folder.RemoveFromEnd(TEXT("/"));
		return Folder + TEXT("/") + InName;
	}

	FString RetargetObjectPath(const FString& InPath, const TMap<FString, FString>& MoveMap)
	{
		if (InPath.IsEmpty() || MoveMap.Num() == 0)
		{
			return FString();
		}

		const FString OldPackage = StripObjectPath(InPath);
		if (OldPackage.IsEmpty())
		{
			return FString();
		}

		const FString* NewPackage = MoveMap.Find(OldPackage);
		if (NewPackage == nullptr || NewPackage->IsEmpty() || *NewPackage == OldPackage)
		{
			return FString();
		}

		// InPath 去掉旧包名前缀之后的部分就是对象/子对象后缀 (可能是空串)
		return *NewPackage + InPath.RightChop(OldPackage.Len());
	}

	bool IsRedirector(const FAssetData& Asset)
	{
		if (!Asset.IsValid())
		{
			return false;
		}
		if (Asset.IsRedirector())
		{
			return true;
		}
		const UClass* AssetClass = Asset.GetClass();
		return AssetClass != nullptr && AssetClass->IsChildOf(UObjectRedirector::StaticClass());
	}

	bool IsUnderPackagePath(const FString& PackageName, const FString& PackagePath)
	{
		if (PackageName.IsEmpty() || PackagePath.IsEmpty())
		{
			return false;
		}
		FString NormalizedPath = PackagePath;
		NormalizedPath.RemoveFromEnd(TEXT("/"));
		if (PackageName.Equals(NormalizedPath, ESearchCase::CaseSensitive))
		{
			return true;
		}
		return PackageName.StartsWith(NormalizedPath + TEXT("/"), ESearchCase::CaseSensitive);
	}

	bool IsInternalPackagePath(const FString& PackageName)
	{
		return PackageName.Contains(TEXT("/__ExternalActors__/"), ESearchCase::CaseSensitive)
			|| PackageName.Contains(TEXT("/__ExternalObjects__/"), ESearchCase::CaseSensitive);
	}

	bool IsNonAssetPackagePath(const FString& PackageName)
	{
		if (PackageName.IsEmpty())
		{
			return true;
		}

		// /Script/Engine 这类是 C++ 原生类所在的编译期包, 永远不是资产
		if (PackageName.StartsWith(TEXT("/Script"), ESearchCase::CaseSensitive))
		{
			// 注意要区分 /Script 与 /Scripts 之类, 所以检查后面必须是 '/' 或结束
			if (PackageName.Len() == 7 || PackageName[7] == TEXT('/'))
			{
				return true;
			}
		}

		static const TCHAR* NonAssetRoots[] = {
			TEXT("/Temp/"),
			TEXT("/Transient/"),
			TEXT("/Engine/Transient"),
			TEXT("/Game/Transient"),
		};
		for (const TCHAR* Root : NonAssetRoots)
		{
			if (PackageName.StartsWith(Root, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}

		return false;
	}

	bool DoesAssetExistOnDisk(const FString& PackageOrObjectPath)
	{
		const FString PackageName = StripObjectPath(PackageOrObjectPath);
		if (PackageName.IsEmpty())
		{
			return false;
		}
		return FPackageName::DoesPackageExist(PackageName);
	}

	bool DoesAssetExistInRegistry(const FString& PackageOrObjectPath, bool bIncludeRedirectors)
	{
		const FString PackageName = StripObjectPath(PackageOrObjectPath);
		if (PackageName.IsEmpty())
		{
			return false;
		}

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> Assets;
		AssetRegistryModule.Get().GetAssetsByPackageName(FName(*PackageName), Assets);

		if (bIncludeRedirectors)
		{
			return Assets.Num() > 0;
		}
		for (const FAssetData& Asset : Assets)
		{
			if (!IsRedirector(Asset))
			{
				return true;
			}
		}
		return false;
	}

	bool IsAssetPathFree(const FString& PackageOrObjectPath)
	{
		return !DoesAssetExistOnDisk(PackageOrObjectPath) && !DoesAssetExistInRegistry(PackageOrObjectPath, true);
	}

	bool TryMakeUniqueAssetPath(const FString& DesiredFolder, const FString& DesiredName,
		FString& OutFolder, FString& OutName)
	{
		OutFolder.Reset();
		OutName.Reset();

		if (DesiredFolder.IsEmpty() || DesiredName.IsEmpty())
		{
			return false;
		}

		const FString DesiredPackage = CombinePackagePath(DesiredFolder, DesiredName);
		if (IsAssetPathFree(DesiredPackage))
		{
			OutFolder = DesiredFolder;
			OutName = DesiredName;
			return true;
		}

		// 先让引擎基于资产注册表挑一个唯一名
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		FString UniquePackage;
		FString UniqueName;
		AssetToolsModule.Get().CreateUniqueAssetName(DesiredPackage, TEXT(""), UniquePackage, UniqueName);
		if (!UniquePackage.IsEmpty() && IsAssetPathFree(UniquePackage))
		{
			OutFolder = FPackageName::GetLongPackagePath(UniquePackage);
			OutName = UniqueName;
			return true;
		}

		// 注册表看不见但磁盘确实存在的"幽灵"资产: 手工递增
		for (int32 Counter = 1; Counter <= 999; ++Counter)
		{
			const FString CandidateName = FString::Printf(TEXT("%s_%d"), *DesiredName, Counter);
			const FString CandidatePackage = CombinePackagePath(DesiredFolder, CandidateName);
			if (IsAssetPathFree(CandidatePackage))
			{
				OutFolder = DesiredFolder;
				OutName = CandidateName;
				return true;
			}
		}

		return false;
	}

	bool TryConvertFilenameToPackagePath(const FString& InFilename, FString& OutPackagePath)
	{
		OutPackagePath.Reset();
		if (InFilename.IsEmpty())
		{
			return false;
		}
		FString PackageName;
		if (!FPackageName::TryConvertFilenameToLongPackageName(InFilename, PackageName))
		{
			return false;
		}
		OutPackagePath = PackageName;
		return true;
	}

	FString GetAssetDiskFilename(const FString& PackageName, bool bIsWorld)
	{
		FString Filename;
		if (FPackageName::DoesPackageExist(PackageName, &Filename) && !Filename.IsEmpty())
		{
			return FPaths::ConvertRelativePathToFull(Filename);
		}
		const FString Extension = bIsWorld ? TEXT(".umap") : TEXT(".uasset");
		return FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(PackageName, Extension));
	}

	FString GetGameRootPath()
	{
		return TEXT("/Game");
	}

	bool MakeSuffixedName(const FString& InName, const FString& Suffix, FString& OutName)
	{
		OutName.Reset();
		const FString TrimmedSuffix = Suffix.TrimStartAndEnd();
		if (TrimmedSuffix.IsEmpty() || InName.IsEmpty())
		{
			return false;
		}

		const FString SuffixWithUnderscore = TEXT("_") + TrimmedSuffix;
		if (InName.EndsWith(SuffixWithUnderscore, ESearchCase::CaseSensitive))
		{
			// 幂等: 已经带了这个后缀, 不重复叠加
			return false;
		}

		OutName = InName + SuffixWithUnderscore;
		return true;
	}

	bool MakeUnsuffixedName(const FString& InName, const FString& Suffix, FString& OutName)
	{
		OutName.Reset();
		const FString TrimmedSuffix = Suffix.TrimStartAndEnd();
		if (TrimmedSuffix.IsEmpty() || InName.IsEmpty())
		{
			return false;
		}

		const FString SuffixWithUnderscore = TEXT("_") + TrimmedSuffix;
		if (!InName.EndsWith(SuffixWithUnderscore, ESearchCase::CaseSensitive))
		{
			return false;
		}

		const FString Result = InName.LeftChop(SuffixWithUnderscore.Len());
		if (Result.IsEmpty())
		{
			return false;
		}

		OutName = Result;
		return true;
	}

	bool MakeUnsuffixedNameLegacy(const FString& InName, FString& OutName)
	{
		OutName.Reset();
		if (InName.IsEmpty())
		{
			return false;
		}

		int32 SeparatorIndex = INDEX_NONE;
		if (!InName.FindLastChar(TEXT('_'), SeparatorIndex))
		{
			return false;
		}
		// 原实现允许把 "_abc" 变成空名, 这里收紧为不处理
		if (SeparatorIndex <= 0)
		{
			return false;
		}

		OutName = InName.Left(SeparatorIndex);
		return !OutName.IsEmpty();
	}
}
