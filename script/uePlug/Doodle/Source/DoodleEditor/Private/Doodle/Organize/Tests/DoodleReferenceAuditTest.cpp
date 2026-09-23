// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Doodle/Organize/DoodleReferenceAudit.h"

#include "Engine/PrimaryAssetLabel.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

namespace
{
	using FAudit = FDoodleReferenceAudit;

	FName N(const TCHAR* Name)
	{
		return FName(Name);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleReferenceAuditDiffTest,
	"Doodle.Organize.ReferenceAudit.Diff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleReferenceAuditDiffTest::RunTest(const FString& Parameters)
{
	// Before:  A->B, A->C, D->F
	FAudit::FSnapshot Before;
	Before.TakenAt = FDateTime::UtcNow();
	Before.RootPath = TEXT("/Game");
	Before.Packages = { N(TEXT("/Game/A")), N(TEXT("/Game/B")), N(TEXT("/Game/C")), N(TEXT("/Game/D")), N(TEXT("/Game/F")) };
	Before.HardEdges = {
		{ N(TEXT("/Game/A")), N(TEXT("/Game/B")) },
		{ N(TEXT("/Game/A")), N(TEXT("/Game/C")) },
		{ N(TEXT("/Game/D")), N(TEXT("/Game/F")) },
	};

	// After:  A->B2 (B 被搬到 B2 且引用已修), E->A (新增)
	//         A->C 丢失 (C 仍在)      -> LostEdges
	//         D->F 丢失 (F 也不在了)  -> LostDependencies
	FAudit::FSnapshot After;
	After.TakenAt = FDateTime::UtcNow();
	After.RootPath = TEXT("/Game");
	After.Packages = { N(TEXT("/Game/A")), N(TEXT("/Game/B2")), N(TEXT("/Game/C")), N(TEXT("/Game/E")) };
	After.HardEdges = {
		{ N(TEXT("/Game/A")), N(TEXT("/Game/B2")) },
		{ N(TEXT("/Game/E")), N(TEXT("/Game/A")) },
	};

	FAudit::FDiffOptions Options;
	Options.bCheckRegistryForExistence = false;
	Options.MoveMap.Add(N(TEXT("/Game/B")), N(TEXT("/Game/B2")));

	const FAudit::FDiffResult Result = FAudit::Diff(Before, After, Options);

	TestEqual(TEXT("真丢失数量"), Result.LostEdges.Num(), 1);
	if (Result.LostEdges.Num() == 1)
	{
		TestTrue(TEXT("真丢失的引用者应为 /Game/A"), Result.LostEdges[0].Package == N(TEXT("/Game/A")));
		TestTrue(TEXT("真丢失的依赖应为 /Game/C"), Result.LostEdges[0].Dependency == N(TEXT("/Game/C")));
	}

	TestEqual(TEXT("依赖消失数量"), Result.LostDependencies.Num(), 1);
	if (Result.LostDependencies.Num() == 1)
	{
		TestTrue(TEXT("依赖消失的依赖应为 /Game/F"), Result.LostDependencies[0].Dependency == N(TEXT("/Game/F")));
	}

	TestEqual(TEXT("正确改指数量"), Result.RetargetedEdges.Num(), 1);
	if (Result.RetargetedEdges.Num() == 1)
	{
		TestTrue(TEXT("改指后的新依赖应为 /Game/B2"), Result.RetargetedEdges[0].RetargetedTo == N(TEXT("/Game/B2")));
	}

	TestEqual(TEXT("新增边数量"), Result.AddedEdges.Num(), 1);
	TestTrue(TEXT("HasRealLoss"), Result.HasRealLoss());
	TestTrue(TEXT("HasAnyLoss"), Result.HasAnyLoss());
	TestTrue(TEXT("HasAnyChange"), Result.HasAnyChange());
	TestEqual(TEXT("边数快照 (前)"), Result.NumEdgesBefore, 3);
	TestEqual(TEXT("边数快照 (后)"), Result.NumEdgesAfter, 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleReferenceAuditSmokingGunTest,
	"Doodle.Organize.ReferenceAudit.MovedButNotFixed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleReferenceAuditSmokingGunTest::RunTest(const FString& Parameters)
{
	// 这就是「整理文件后引用丢失」的典型形态:
	// 依赖被搬到了新路径, 但引用没有被修复 —— 必须被判成真·引用丢失, 而不是"正常改指"。
	FAudit::FSnapshot Before;
	Before.RootPath = TEXT("/Game");
	Before.Packages = { N(TEXT("/Game/Char/Mesh")), N(TEXT("/Game/Tex/T_Rock")) };
	Before.HardEdges = {
		{ N(TEXT("/Game/Char/Mesh")), N(TEXT("/Game/Tex/T_Rock")) },
	};

	FAudit::FSnapshot After;
	After.RootPath = TEXT("/Game");
	After.Packages = { N(TEXT("/Game/Char/Mesh")), N(TEXT("/Game/Char/Tex/T_Rock")) };
	After.HardEdges = {
		// 注意: Mesh 对新路径没有任何引用
	};

	FAudit::FDiffOptions Options;
	Options.bCheckRegistryForExistence = false;
	Options.MoveMap.Add(N(TEXT("/Game/Tex/T_Rock")), N(TEXT("/Game/Char/Tex/T_Rock")));

	const FAudit::FDiffResult Result = FAudit::Diff(Before, After, Options);

	TestEqual(TEXT("真丢失数量"), Result.LostEdges.Num(), 1);
	TestEqual(TEXT("正确改指数量"), Result.RetargetedEdges.Num(), 0);
	TestTrue(TEXT("应判定为真丢失"), Result.HasRealLoss());
	if (Result.LostEdges.Num() == 1)
	{
		TestTrue(TEXT("应记录改指目标"), Result.LostEdges[0].RetargetedTo == N(TEXT("/Game/Char/Tex/T_Rock")));
		TestTrue(TEXT("应带说明"), !Result.LostEdges[0].Note.IsEmpty());
	}

	// 反向验证: 如果引用确实跟着改了, 就不该报丢失
	After.HardEdges.Add({ N(TEXT("/Game/Char/Mesh")), N(TEXT("/Game/Char/Tex/T_Rock")) });
	const FAudit::FDiffResult FixedResult = FAudit::Diff(Before, After, Options);
	TestEqual(TEXT("修复后不应有真丢失"), FixedResult.LostEdges.Num(), 0);
	TestEqual(TEXT("修复后应记 1 条正确改指"), FixedResult.RetargetedEdges.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleReferenceAuditSnapshotIoTest,
	"Doodle.Organize.ReferenceAudit.SnapshotIo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleReferenceAuditSnapshotIoTest::RunTest(const FString& Parameters)
{
	FAudit::FSnapshot Original;
	Original.TakenAt = FDateTime(2024, 5, 17, 10, 30, 0);
	Original.RootPath = TEXT("/Game");
	Original.Packages = { N(TEXT("/Game/A")), N(TEXT("/Game/B")) };
	Original.HardEdges = { { N(TEXT("/Game/A")), N(TEXT("/Game/B")) } };
	Original.SoftEdges = { { N(TEXT("/Game/B")), N(TEXT("/Engine/EngineMaterials/DefaultMaterial")) } };

	const FString Filename = FPaths::ProjectSavedDir() / TEXT("DoodleOrganize") / TEXT("AutomationTest_Snapshot.json");

	TestTrue(TEXT("快照应能写入"), Original.Save(Filename));

	FAudit::FSnapshot Loaded;
	TestTrue(TEXT("快照应能读回"), FAudit::FSnapshot::Load(Filename, Loaded));

	TestEqual(TEXT("RootPath 应一致"), Loaded.RootPath, Original.RootPath);
	TestEqual(TEXT("包数量应一致"), Loaded.Packages.Num(), Original.Packages.Num());
	TestEqual(TEXT("硬边数量应一致"), Loaded.HardEdges.Num(), Original.HardEdges.Num());
	TestEqual(TEXT("软边数量应一致"), Loaded.SoftEdges.Num(), Original.SoftEdges.Num());
	if (Loaded.HardEdges.Num() == 1)
	{
		TestTrue(TEXT("硬边内容应一致"), Loaded.HardEdges[0] == Original.HardEdges[0]);
	}
	if (Loaded.SoftEdges.Num() == 1)
	{
		TestTrue(TEXT("软边内容应一致"), Loaded.SoftEdges[0] == Original.SoftEdges[0]);
	}

	// 报告渲染不应崩溃
	const FAudit::FDiffResult Result = FAudit::Diff(Original, Loaded);
	TestFalse(TEXT("同一快照比对不应有变化"), Result.HasAnyChange());
	TestTrue(TEXT("ToText 应有内容"), !Result.ToText().IsEmpty());
	TestTrue(TEXT("ToCsv 应有表头"), Result.ToCsv().StartsWith(TEXT("kind,package,dependency")));

	// 报告目录应能被创建
	TestTrue(TEXT("报告目录应存在"), !FAudit::GetReportDirectory().IsEmpty());

	return true;
}

// ---------------------------------------------------------------------------
// 根因修复的端到端测试: 真的改写内存里的软引用
//
// 前面的测试都是纯逻辑; 这个测试造一个「内存里的包 + 带软引用的资产」,
// 然后调用 RetargetSoftReferences, 断言:
//   1. 命中 MoveMap 的软引用被真的改写了;
//   2. 没命中的软引用一根汗毛都不动;
//   3. 被改写的包被标脏 (否则改动不会落盘);
//   4. 再跑一次是幂等的 (不会改回来, 也不会重复计数)。
//
// 用 UPrimaryAssetLabel 是因为它有 TArray<TSoftObjectPtr<UObject>> (ExplicitAssets)
// 和 TArray<TSoftClassPtr<UObject>> (ExplicitBlueprints), 正好覆盖
// FArrayProperty -> FSoftObjectProperty / FSoftClassProperty 这条路。
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleReferenceAuditRetargetLiveTest,
	"Doodle.Organize.ReferenceAudit.RetargetLive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleReferenceAuditRetargetLiveTest::RunTest(const FString& Parameters)
{
	const FString TestRoot = TEXT("/Game/DoodleOrganizeTest_RetargetLive");
	const FString OldPackage = TestRoot + TEXT("/Old/T_rock");
	const FString NewPackage = TestRoot + TEXT("/Tex/T_rock");
	const FString OldObjectPath = OldPackage + TEXT(".T_rock");
	const FString NewObjectPath = NewPackage + TEXT(".T_rock");

	// 不参与比对的软引用: 名字很像, 但不在 MoveMap 里, 必须保持原样
	const FString UntouchedObjectPath = TestRoot + TEXT("/Old/T_rock_2.T_rock_2");

	// ---- 造一个内存里的包 (不落盘) ----
	UPackage* Package = CreatePackage(*TestRoot);
	if (Package == nullptr)
	{
		AddError(TEXT("无法创建测试包"));
		return false;
	}

	UPrimaryAssetLabel* Label = NewObject<UPrimaryAssetLabel>(
		Package, TEXT("RetargetLiveLabel"), RF_Public | RF_Transactional);
	if (Label == nullptr)
	{
		AddError(TEXT("无法创建测试资产"));
		return false;
	}

	Label->ExplicitAssets.Add(TSoftObjectPtr<UObject>(FSoftObjectPath(OldObjectPath)));
	Label->ExplicitAssets.Add(TSoftObjectPtr<UObject>(FSoftObjectPath(UntouchedObjectPath)));
	Label->ExplicitBlueprints.Add(TSoftClassPtr<UObject>(
		FSoftObjectPath(TestRoot + TEXT("/Old/BP_Thing.BP_Thing_C"))));

	Package->SetDirtyFlag(false);

	// 先确认遍历器能看到这个包 (否则后面的断言等于没测)
	{
		const FAudit::FSoftScanStats Before = FAudit::ScanSoftReferences(TestRoot);
		TestTrue(TEXT("遍历器应能看到测试包"), Before.NumObjectsVisited > 0);
		TestTrue(TEXT("遍历器应能看到软引用属性"), Before.NumSoftPropertiesSeen > 0);
		TestEqual(TEXT("遍历器应看到 3 条命中根路径的软引用 (2 条对象 + 1 条类)"),
			Before.NumSoftPathsSeen, 3);
	}

	// ---- 执行补修 ----
	TMap<FName, FName> MoveMap;
	MoveMap.Add(N(*OldPackage), N(*NewPackage));

	TArray<FString> Touched;
	const int32 NumRetargeted = FAudit::RetargetSoftReferences(MoveMap, TestRoot, Touched);

	TestEqual(TEXT("应补修 1 条软引用"), NumRetargeted, 1);
	TestEqual(TEXT("应记录 1 个待保存的包"), Touched.Num(), 1);
	if (Touched.Num() == 1)
	{
		TestEqual(TEXT("待保存的包名"), Touched[0], TestRoot);
	}

	// ---- 关键: 属性真的被改写了吗 ----
	TestEqual(TEXT("ExplicitAssets 数量不变"), Label->ExplicitAssets.Num(), 2);
	if (Label->ExplicitAssets.Num() == 2)
	{
		TestEqual(TEXT("命中的软引用应改写成新路径"),
			Label->ExplicitAssets[0].ToSoftObjectPath().ToString(), NewObjectPath);
		TestEqual(TEXT("没命中的软引用必须原样不动 (同前缀不误伤)"),
			Label->ExplicitAssets[1].ToSoftObjectPath().ToString(), UntouchedObjectPath);
	}
	if (Label->ExplicitBlueprints.Num() == 1)
	{
		TestEqual(TEXT("没命中的软类引用必须原样不动"),
			Label->ExplicitBlueprints[0].ToSoftObjectPath().ToString(),
			TestRoot + TEXT("/Old/BP_Thing.BP_Thing_C"));
	}

	// ---- 被改写的包必须被标脏, 否则改动不会落盘 ----
	TestTrue(TEXT("被改写的包应被标脏"), Package->IsDirty());

	// ---- 幂等: 再跑一次不应该有任何改动 ----
	TArray<FString> TouchedAgain;
	const int32 NumRetargetedAgain = FAudit::RetargetSoftReferences(MoveMap, TestRoot, TouchedAgain);
	TestEqual(TEXT("重复补修应为幂等 (0 条)"), NumRetargetedAgain, 0);
	TestEqual(TEXT("重复补修不应记录待保存的包"), TouchedAgain.Num(), 0);
	TestEqual(TEXT("重复补修后路径不应变化"),
		Label->ExplicitAssets[0].ToSoftObjectPath().ToString(), NewObjectPath);

	// ---- 清理: 别让这个测试包留在内存里等着被保存 ----
	Package->SetDirtyFlag(false);
	Package->MarkAsGarbage();

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
