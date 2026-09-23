// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/DoodleOrganizeRules.h"

#include "Doodle/Organize/DoodleAssetPathUtils.h"

#include "Animation/AnimationAsset.h"
#include "Animation/Skeleton.h"
#include "Engine/Blueprint.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "Particles/ParticleSystem.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/ObjectRedirector.h"

namespace DoodleOrganize
{
	namespace
	{
		bool ClassPathMatches(const UClass* AssetClass, const FString& ClassPathString)
		{
			return AssetClass != nullptr
				&& AssetClass->GetClassPathName().ToString().Equals(ClassPathString, ESearchCase::IgnoreCase);
		}
	}

	bool TryResolveSubFolder(const FAssetData& Asset, const FRuleSet& Rules,
		FString& OutSubFolder, FString& OutSkipReason)
	{
		OutSubFolder.Reset();
		OutSkipReason.Reset();

		if (!Asset.IsValid())
		{
			OutSkipReason = TEXT("资产数据无效");
			return false;
		}

		if (IsRedirector(Asset))
		{
			OutSkipReason = TEXT("重定向器不参与整理");
			return false;
		}

		const FString PackageName = Asset.PackageName.ToString();
		if (IsInternalPackagePath(PackageName))
		{
			OutSkipReason = TEXT("World Partition 内部目录 (__ExternalActors__/__ExternalObjects__) 不参与整理");
			return false;
		}

		const UClass* AssetClass = Asset.GetClass();
		if (AssetClass == nullptr)
		{
			OutSkipReason = TEXT("无法解析资产类型 (类未加载)");
			return false;
		}

		for (const FString& ExcludedClassPath : Rules.AlwaysExcludedClasses)
		{
			if (ClassPathMatches(AssetClass, ExcludedClassPath))
			{
				OutSkipReason = FString::Printf(TEXT("类型 %s 在强制排除表中"), *AssetClass->GetName());
				return false;
			}
		}

		for (const FString& ExcludedClassPath : Rules.ExtraExcludedClasses)
		{
			if (ClassPathMatches(AssetClass, ExcludedClassPath))
			{
				OutSkipReason = FString::Printf(TEXT("类型 %s 在用户排除表中"), *AssetClass->GetName());
				return false;
			}
		}

		for (const FTypeRule& Rule : Rules.Rules)
		{
			if (Rule.Class == nullptr || Rule.SubFolder == nullptr)
			{
				continue;
			}

			const bool bMatched = Rule.bExactClass
				? (AssetClass == Rule.Class)
				: AssetClass->IsChildOf(Rule.Class);
			if (!bMatched)
			{
				continue;
			}

			if (Rule.Class == UWorld::StaticClass() && !Rules.bIncludeWorlds)
			{
				OutSkipReason = TEXT("设置中已关闭地图整理");
				return false;
			}

			OutSubFolder = Rule.SubFolder;
			return true;
		}

		if (Rules.bIncludeOtherTypes)
		{
			OutSubFolder = TEXT("Other");
			return true;
		}

		OutSkipReason = FString::Printf(TEXT("类型 %s 未命中规则且未启用 Other 兜底"), *AssetClass->GetName());
		return false;
	}

	const FRuleSet& GetAssetRules()
	{
		// 原 GenerateFolders (第 468-542 行) 的映射。
		// 修正: 材质相关的四类由精确类比较改为 IsChildOf, 使 UMaterialInstanceConstant
		//       等子类不再落到 Other。
		static const FRuleSet Rules = []()
		{
			FRuleSet Result;
			Result.Name = TEXT("Asset");
			Result.Rules = {
				{ UFXSystemAsset::StaticClass(),               false, TEXT("Fx") },
				{ UStaticMesh::StaticClass(),                  false, TEXT("Mesh") },
				{ UTexture::StaticClass(),                     false, TEXT("Tex") },
				{ UMaterialInstance::StaticClass(),            false, TEXT("Mat/MatInst") },
				{ UMaterial::StaticClass(),                    false, TEXT("Mat/Mat") },
				{ UMaterialParameterCollection::StaticClass(), false, TEXT("Mat/MatParSet") },
				{ UMaterialFunction::StaticClass(),            false, TEXT("Mat/MatFun") },
				{ UWorld::StaticClass(),                       false, TEXT("Maps") },
				{ UBlueprint::StaticClass(),                   false, TEXT("Blue") },
			};
			Result.AlwaysExcludedClasses = { UObjectRedirector::StaticClass()->GetClassPathName().ToString() };
			Result.bIncludeOtherTypes = true;
			Result.bIncludeWorlds = true;
			return Result;
		}();
		return Rules;
	}

	const FRuleSet& GetCharacterRules()
	{
		// 原 GenerateModeFolders (第 924-995 行) 的映射。
		static const FRuleSet Rules = []()
		{
			FRuleSet Result;
			Result.Name = TEXT("Character");
			Result.Rules = {
				{ UStaticMesh::StaticClass(),                  false, TEXT("Meshs") },
				{ USkeletalMesh::StaticClass(),                false, TEXT("Meshs") },
				{ UPhysicsAsset::StaticClass(),                false, TEXT("Meshs") },
				{ UAnimationAsset::StaticClass(),              false, TEXT("Meshs") },
				{ USkeleton::StaticClass(),                    false, TEXT("Meshs") },
				{ UTexture::StaticClass(),                     false, TEXT("Texture") },
				{ UMaterialInstance::StaticClass(),            false, TEXT("Material") },
				{ UGroomAsset::StaticClass(),                  false, TEXT("Meshs/Groom") },
				{ UGroomBindingAsset::StaticClass(),           false, TEXT("Meshs/Groom") },
				{ UMaterial::StaticClass(),                    false, TEXT("Material/Mu01") },
				{ UMaterialParameterCollection::StaticClass(), false, TEXT("Material/Mate01") },
				{ UMaterialFunction::StaticClass(),            false, TEXT("Material/Mate01") },
				{ UBlueprint::StaticClass(),                   false, TEXT("Material/Mate01") },
			};
			Result.AlwaysExcludedClasses = { UObjectRedirector::StaticClass()->GetClassPathName().ToString() };
			Result.bIncludeOtherTypes = true;
			Result.bIncludeWorlds = true;
			return Result;
		}();
		return Rules;
	}

	TArray<FString> GetRecommendedExcludedClassPaths()
	{
		// 仅作为设置面板上的建议项; 默认不生效, 由用户决定是否加入 ExtraExcludedClasses。
		return {
			TEXT("/Script/LevelSequence.LevelSequence"),
			TEXT("/Script/Engine.DataAsset"),
			TEXT("/Script/Niagara.NiagaraSystem"),
		};
	}
}
