// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/DoodleRedirectFixup.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "UObject/ObjectRedirector.h"

namespace DoodleOrganize
{
	TArray<FString> FindRedirectorsUnderPath(const FString& PackagePath)
	{
		TArray<FString> Result;
		if (PackagePath.IsEmpty())
		{
			return Result;
		}

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		FARFilter Filter;
		Filter.bRecursivePaths = true;
		Filter.bRecursiveClasses = true;
		Filter.PackagePaths.Add(FName(*PackagePath));
		Filter.ClassPaths.Add(UObjectRedirector::StaticClass()->GetClassPathName());

		TArray<FAssetData> Assets;
		AssetRegistryModule.Get().GetAssets(Filter, Assets);

		Result.Reserve(Assets.Num());
		for (const FAssetData& Asset : Assets)
		{
			Result.Add(Asset.GetObjectPathString());
		}
		return Result;
	}

	FDoodleOrganizeReport FixupRedirectorsForPaths(
		const TArray<FString>& RedirectorPackagePaths, ERedirectFixupMode Mode)
	{
		FDoodleOrganizeReport Report;
		Report.Operation = TEXT("FixupRedirectors");
		Report.NumRequested = RedirectorPackagePaths.Num();

		if (RedirectorPackagePaths.Num() == 0)
		{
			Report.Finalize();
			return Report;
		}

		TArray<UObjectRedirector*> Redirectors;
		Redirectors.Reserve(RedirectorPackagePaths.Num());

		for (const FString& ObjectPath : RedirectorPackagePaths)
		{
			// 关键: LOAD_NoRedirects —— 不要跟随重定向器拿到目标对象
			UObject* Object = LoadObject<UObject>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_NoRedirects);
			UObjectRedirector* Redirector = Cast<UObjectRedirector>(Object);
			if (Redirector != nullptr)
			{
				Redirectors.Add(Redirector);
			}
			else
			{
				Report.AddSkip(FString::Printf(TEXT("%s : 不是可加载的重定向器"), *ObjectPath));
			}
		}

		if (Redirectors.Num() == 0)
		{
			Report.Finalize();
			return Report;
		}

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetToolsModule.Get().FixupReferencers(Redirectors, /*bCheckoutDialogPrompt*/ false, Mode);

		Report.NumMoved = Redirectors.Num();

		if (Mode == ERedirectFixupMode::DeleteFixedUpRedirectors)
		{
			Report.AddWarning(TEXT("已按 DeleteFixedUpRedirectors 模式删除重定向器, 旧路径不再有引用兜底"));
		}

		Report.Finalize();
		return Report;
	}
}
