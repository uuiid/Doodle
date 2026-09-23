// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Doodle/Organize/DoodleOrganizeRules.h"

#include "Engine/Blueprint.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialParameterCollection.h"
#include "UObject/ObjectRedirector.h"

namespace
{
	FAssetData MakeAssetData(const TCHAR* PackagePath, const TCHAR* AssetName, const UClass* AssetClass)
	{
		const FString PackageName = FString::Printf(TEXT("%s/%s"), PackagePath, AssetName);
		return FAssetData(FName(*PackageName), FName(PackagePath), FName(AssetName), AssetClass->GetClassPathName());
	}

	/** 断言某个类被映射到期望的子目录 */
	void CheckMapping(FAutomationTestBase& Test, const TCHAR* AssetName, const UClass* AssetClass,
		const TCHAR* ExpectedSubFolder)
	{
		const FAssetData Asset = MakeAssetData(TEXT("/Game/Input"), AssetName, AssetClass);
		FString SubFolder;
		FString SkipReason;
		const bool bResolved = DoodleOrganize::TryResolveSubFolder(Asset, DoodleOrganize::GetAssetRules(), SubFolder, SkipReason);

		Test.TestTrue(FString::Printf(TEXT("%s 应命中规则"), AssetName), bResolved);
		if (bResolved)
		{
			Test.TestEqual(FString::Printf(TEXT("%s 的子目录"), AssetName), SubFolder, FString(ExpectedSubFolder));
		}
		else
		{
			Test.AddError(FString::Printf(TEXT("%s 未命中, 原因: %s"), AssetName, *SkipReason));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleOrganizeRulesMappingTest,
	"Doodle.Organize.Rules.Mapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleOrganizeRulesMappingTest::RunTest(const FString& Parameters)
{
	// ---- 「整理选中资源」映射表 (与原 GenerateFolders 逐条对照) ----
	CheckMapping(*this, TEXT("SM_Rock"), UStaticMesh::StaticClass(), TEXT("Mesh"));
	CheckMapping(*this, TEXT("T_Rock"), UTexture2D::StaticClass(), TEXT("Tex"));
	CheckMapping(*this, TEXT("M_Base"), UMaterial::StaticClass(), TEXT("Mat/Mat"));
	CheckMapping(*this, TEXT("MPC_Global"), UMaterialParameterCollection::StaticClass(), TEXT("Mat/MatParSet"));
	CheckMapping(*this, TEXT("MF_Noise"), UMaterialFunction::StaticClass(), TEXT("Mat/MatFun"));
	CheckMapping(*this, TEXT("BP_Actor"), UBlueprint::StaticClass(), TEXT("Blue"));
	CheckMapping(*this, TEXT("Map_Level"), UWorld::StaticClass(), TEXT("Maps"));

	// 修正点: 材质实例的子类不再落到 Other
	CheckMapping(*this, TEXT("MI_Base"), UMaterialInstanceConstant::StaticClass(), TEXT("Mat/MatInst"));

	// 未命中规则 -> Other
	CheckMapping(*this, TEXT("DA_Config"), UDataAsset::StaticClass(), TEXT("Other"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleOrganizeRulesExclusionTest,
	"Doodle.Organize.Rules.Exclusion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleOrganizeRulesExclusionTest::RunTest(const FString& Parameters)
{
	FString SubFolder;
	FString SkipReason;

	// ---- 重定向器永远跳过 ----
	{
		const FAssetData Asset = MakeAssetData(TEXT("/Game/Input"), TEXT("Redirector"), UObjectRedirector::StaticClass());
		TestFalse(TEXT("重定向器不应命中规则"),
			DoodleOrganize::TryResolveSubFolder(Asset, DoodleOrganize::GetAssetRules(), SubFolder, SkipReason));
	}

	// ---- World Partition 内部目录永远跳过 ----
	{
		const FAssetData Asset = MakeAssetData(TEXT("/Game/__ExternalActors__/Map/AB/CD"), TEXT("CD"), UTexture2D::StaticClass());
		TestFalse(TEXT("__ExternalActors__ 下的包不应命中规则"),
			DoodleOrganize::TryResolveSubFolder(Asset, DoodleOrganize::GetAssetRules(), SubFolder, SkipReason));
	}
	{
		const FAssetData Asset = MakeAssetData(TEXT("/Game/__ExternalObjects__/Map/AB/CD"), TEXT("CD"), UTexture2D::StaticClass());
		TestFalse(TEXT("__ExternalObjects__ 下的包不应命中规则"),
			DoodleOrganize::TryResolveSubFolder(Asset, DoodleOrganize::GetAssetRules(), SubFolder, SkipReason));
	}

	// ---- bIncludeWorlds = false 时地图被跳过 ----
	{
		DoodleOrganize::FRuleSet Rules = DoodleOrganize::GetAssetRules();
		Rules.bIncludeWorlds = false;
		const FAssetData Asset = MakeAssetData(TEXT("/Game/Input"), TEXT("Map_Level"), UWorld::StaticClass());
		TestFalse(TEXT("关闭地图整理后 UWorld 应被跳过"),
			DoodleOrganize::TryResolveSubFolder(Asset, Rules, SubFolder, SkipReason));
	}

	// ---- bIncludeOtherTypes = false 时未命中类型被跳过 ----
	{
		DoodleOrganize::FRuleSet Rules = DoodleOrganize::GetAssetRules();
		Rules.bIncludeOtherTypes = false;
		const FAssetData Asset = MakeAssetData(TEXT("/Game/Input"), TEXT("DA_Config"), UDataAsset::StaticClass());
		TestFalse(TEXT("关闭 Other 兜底后未命中类型应被跳过"),
			DoodleOrganize::TryResolveSubFolder(Asset, Rules, SubFolder, SkipReason));
	}

	// ---- 用户额外排除表 ----
	{
		DoodleOrganize::FRuleSet Rules = DoodleOrganize::GetAssetRules();
		Rules.ExtraExcludedClasses.Add(UStaticMesh::StaticClass()->GetClassPathName().ToString());
		const FAssetData Asset = MakeAssetData(TEXT("/Game/Input"), TEXT("SM_Rock"), UStaticMesh::StaticClass());
		TestFalse(TEXT("用户排除表命中时应跳过"),
			DoodleOrganize::TryResolveSubFolder(Asset, Rules, SubFolder, SkipReason));
	}

	// ---- 无效资产 ----
	{
		const FAssetData Empty;
		TestFalse(TEXT("无效资产不应命中规则"),
			DoodleOrganize::TryResolveSubFolder(Empty, DoodleOrganize::GetAssetRules(), SubFolder, SkipReason));
	}

	// ---- 角色规则表也要覆盖到关键映射 ----
	{
		const FAssetData Asset = MakeAssetData(TEXT("/Game/Input"), TEXT("MI_Base"), UMaterialInstanceConstant::StaticClass());
		FString CharacterSubFolder;
		TestTrue(TEXT("角色规则: 材质实例应命中"),
			DoodleOrganize::TryResolveSubFolder(Asset, DoodleOrganize::GetCharacterRules(), CharacterSubFolder, SkipReason));
		TestEqual(TEXT("角色规则: 材质实例子目录"), CharacterSubFolder, FString(TEXT("Material")));
	}
	{
		const FAssetData Asset = MakeAssetData(TEXT("/Game/Input"), TEXT("SM_Rock"), UStaticMesh::StaticClass());
		FString CharacterSubFolder;
		TestTrue(TEXT("角色规则: 静态网格应命中"),
			DoodleOrganize::TryResolveSubFolder(Asset, DoodleOrganize::GetCharacterRules(), CharacterSubFolder, SkipReason));
		TestEqual(TEXT("角色规则: 静态网格子目录"), CharacterSubFolder, FString(TEXT("Meshs")));
	}

	// ---- 规则表里的类必须都能解析 (防止类路径写错导致规则静默失效) ----
	{
		const auto CheckRuleSet = [this](const DoodleOrganize::FRuleSet& Rules)
		{
			for (const DoodleOrganize::FTypeRule& Rule : Rules.Rules)
			{
				TestNotNull(FString::Printf(TEXT("规则 %s 的类必须能解析"), Rule.SubFolder), Rule.Class);
			}
		};
		CheckRuleSet(DoodleOrganize::GetAssetRules());
		CheckRuleSet(DoodleOrganize::GetCharacterRules());
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
