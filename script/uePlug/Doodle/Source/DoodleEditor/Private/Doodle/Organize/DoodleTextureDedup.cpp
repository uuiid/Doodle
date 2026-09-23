// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/DoodleTextureDedup.h"

#include "Doodle/Organize/DoodleAssetPathUtils.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "Engine/Texture.h"
#include "Subsystems/EditorAssetSubsystem.h"

TArray<FDoodleTextureDedupService::FDupGroup> FDoodleTextureDedupService::FindDuplicates(
	const FString& RootPath, bool bTexturesOnly) const
{
	TArray<FDupGroup> Result;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName(*RootPath));
	if (bTexturesOnly)
	{
		Filter.ClassPaths.Add(UTexture::StaticClass()->GetClassPathName());
	}

	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssets(Filter, Assets);

	// 一次遍历分组 (键 = "类路径|资产名"), 不再像原实现那样在数组里反复 Remove
	TMap<FString, TArray<FAssetData>> Buckets;
	Buckets.Reserve(Assets.Num());

	for (const FAssetData& Asset : Assets)
	{
		if (!Asset.IsValid() || DoodleOrganize::IsRedirector(Asset))
		{
			continue;
		}
		if (DoodleOrganize::IsInternalPackagePath(Asset.PackageName.ToString()))
		{
			continue;
		}

		const FString Key = Asset.AssetClassPath.ToString() + TEXT("|") + Asset.AssetName.ToString();
		Buckets.FindOrAdd(Key).Add(Asset);
	}

	for (const TPair<FString, TArray<FAssetData>>& Bucket : Buckets)
	{
		if (Bucket.Value.Num() < 2)
		{
			continue;
		}

		// 必须位于不同目录才算重复 (与原实现一致)
		TSet<FName> DistinctPackagePaths;
		for (const FAssetData& Asset : Bucket.Value)
		{
			DistinctPackagePaths.Add(Asset.PackagePath);
		}
		if (DistinctPackagePaths.Num() < 2)
		{
			continue;
		}

		FDupGroup Group;
		Group.AssetName = Bucket.Value[0].AssetName;
		Group.ClassPath = Bucket.Value[0].AssetClassPath;
		Group.Instances = Bucket.Value;
		Group.Instances.Sort([](const FAssetData& A, const FAssetData& B)
		{
			return A.PackageName.LexicalLess(B.PackageName);
		});
		Result.Add(MoveTemp(Group));
	}

	Result.Sort([](const FDupGroup& A, const FDupGroup& B)
	{
		return A.AssetName.LexicalLess(B.AssetName);
	});

	return Result;
}

FDoodleOrganizeReport FDoodleTextureDedupService::Consolidate(const FDupGroup& Group, const FAssetData& Keep) const
{
	FDoodleOrganizeReport Report;
	Report.Operation = TEXT("ConsolidateDuplicates");
	Report.NumRequested = Group.Instances.Num();

	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>() : nullptr;
	if (EditorAssetSubsystem == nullptr)
	{
		Report.Result = EDoodleOrganizeResult::Failed;
		Report.AddFailure(TEXT("无法获取 UEditorAssetSubsystem"));
		return Report;
	}

	if (!Keep.IsValid())
	{
		Report.Result = EDoodleOrganizeResult::InvalidInput;
		Report.AddFailure(TEXT("保留目标无效"));
		return Report;
	}

	UObject* KeepObject = Keep.GetAsset();
	if (KeepObject == nullptr)
	{
		Report.Result = EDoodleOrganizeResult::Failed;
		Report.AddFailure(FString::Printf(TEXT("无法加载保留目标 %s"), *Keep.PackageName.ToString()));
		return Report;
	}

	TArray<UObject*> ToConsolidate;
	ToConsolidate.Reserve(Group.Instances.Num());

	for (const FAssetData& Instance : Group.Instances)
	{
		if (Instance.PackageName == Keep.PackageName)
		{
			continue;
		}

		// 原实现会把 GetAsset() 的 nullptr 直接塞给引擎, 这里过滤掉
		UObject* Object = Instance.GetAsset();
		if (Object == nullptr)
		{
			Report.AddSkip(FString::Printf(TEXT("%s : 无法加载, 已跳过"), *Instance.PackageName.ToString()));
			continue;
		}
		ToConsolidate.Add(Object);
	}

	if (ToConsolidate.Num() == 0)
	{
		Report.AddWarning(TEXT("没有可合并的对象"));
		Report.Finalize();
		return Report;
	}

	if (!EditorAssetSubsystem->ConsolidateAssets(KeepObject, ToConsolidate))
	{
		Report.AddFailure(FString::Printf(TEXT("合并到 %s 失败"), *Keep.PackageName.ToString()));
		Report.Finalize();
		return Report;
	}

	// 只保存真正受影响的包 (原实现会保存所有子节点)
	EditorAssetSubsystem->SaveLoadedAsset(KeepObject);

	Report.NumMoved = ToConsolidate.Num();
	Report.Finalize();
	return Report;
}
