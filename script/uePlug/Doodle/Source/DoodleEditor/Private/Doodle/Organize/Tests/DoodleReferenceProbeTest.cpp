// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Doodle/Organize/DoodleAssetPathUtils.h"
#include "Doodle/Organize/DoodleReferenceAudit.h"

#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

/**
 * 工程级引用取证探针。
 *
 * 为什么需要它: 用 `-ExecCmds="Doodle.Organize.DumpReferences /Game;Quit"` 是没用的 ——
 * -ExecCmds 在第 0 帧就执行了, 那时资产注册表还没扫完, 结果永远是「包 0 / 硬边 0 / 软边 0」,
 * 而且 Quit 也不一定会真的退出。自动化测试跑在注册表就绪之后 (帧 100+), 并且
 * -testexit="Automation Test Queue Empty" 能干净退出, 所以工程级取证走这条路。
 *
 * 默认**跳过** (否则每次跑 Doodle.Organize 套件都会全量扫一遍工程, 太慢)。
 * 显式打开:
 *   UnrealEditor-Cmd.exe <project> -DoodleProbe -ExecCmds="Automation RunTests Doodle.Organize.Probe;Quit" ^
 *       -unattended -nopause -nosplash -NoSound -testexit="Automation Test Queue Empty" -abslog=<log>
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleOrganizeLiveProbeTest,
	"Doodle.Organize.Probe.LiveProject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleOrganizeLiveProbeTest::RunTest(const FString& Parameters)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("DoodleProbe")))
	{
		AddInfo(TEXT("跳过: 未指定 -DoodleProbe (工程级全量扫描, 默认不跑)"));
		return true;
	}

	const FString RootPath = DoodleOrganize::GetGameRootPath();

	// ---- 1) 注册表级引用快照 ----
	FDoodleReferenceAudit::FSnapshot Snapshot;
	const bool bCaptured = FDoodleReferenceAudit::Capture(RootPath, Snapshot);

	AddInfo(FString::Printf(TEXT("[探针] Capture(%s) = %s"), *RootPath, bCaptured ? TEXT("成功") : TEXT("失败")));
	AddInfo(FString::Printf(TEXT("[探针] 包 %d / 硬引用边 %d / 软引用边 %d"),
		Snapshot.Packages.Num(), Snapshot.HardEdges.Num(), Snapshot.SoftEdges.Num()));

	// 注册表里一条边都没有 => 说明扫描还没完成, 这次结果不可信, 必须报出来
	TestTrue(TEXT("注册表里应该至少有一条引用边 (为 0 说明资产扫描未完成)"),
		Snapshot.HardEdges.Num() + Snapshot.SoftEdges.Num() > 0);

	FString Report = FString::Printf(
		TEXT("=== Doodle 引用取证探针 ===\n根路径: %s\n包: %d\n硬引用边: %d\n软引用边: %d\n\n"),
		*RootPath, Snapshot.Packages.Num(), Snapshot.HardEdges.Num(), Snapshot.SoftEdges.Num());

	// ---- 2) 悬空硬引用 ----
	const TArray<FDoodleReferenceAudit::FDiffEntry> Dangling =
		FDoodleReferenceAudit::FindDanglingReferences(RootPath);

	AddInfo(FString::Printf(TEXT("[探针] 悬空引用 %d 条"), Dangling.Num()));
	Report += FString::Printf(TEXT("--- 悬空引用 (依赖包已不存在) %d 条 ---\n"), Dangling.Num());
	for (int32 Index = 0; Index < Dangling.Num() && Index < 300; ++Index)
	{
		const FString Line = FString::Printf(TEXT("%s -> %s\n"),
			*Dangling[Index].Package.ToString(), *Dangling[Index].Dependency.ToString());
		Report += Line;
		AddInfo(FString::Printf(TEXT("[探针] 悬空: %s"), *Line.TrimEnd()));
	}

	// ---- 3) 悬空软引用 (注册表盲区) ----
	const TArray<FDoodleReferenceAudit::FDiffEntry> DanglingSoft =
		FDoodleReferenceAudit::FindDanglingSoftReferences(RootPath, /*bDirtyPackagesOnly*/ false);

	// 遍历器实际覆盖的范围 —— 如果 NumSoftPathsSeen 是 0, 补修功能必然是空操作
	const FDoodleReferenceAudit::FSoftScanStats Stats =
		FDoodleReferenceAudit::ScanSoftReferences(RootPath, /*bDirtyPackagesOnly*/ false);

	AddInfo(FString::Printf(
		TEXT("[探针] 软引用遍历器: 包 %d / 对象 %d / 软引用属性 %d / 命中根路径的软引用 %d"),
		Stats.NumPackagesVisited, Stats.NumObjectsVisited,
		Stats.NumSoftPropertiesSeen, Stats.NumSoftPathsSeen));

	AddInfo(FString::Printf(TEXT("[探针] 悬空软引用 %d 条 (含已保存的包)"), DanglingSoft.Num()));
	Report += FString::Printf(
		TEXT("\n--- 软引用遍历器覆盖: 包 %d / 对象 %d / 软引用属性 %d / 命中根路径 %d ---\n"),
		Stats.NumPackagesVisited, Stats.NumObjectsVisited,
		Stats.NumSoftPropertiesSeen, Stats.NumSoftPathsSeen);
	Report += FString::Printf(TEXT("\n--- 悬空软引用 %d 条 ---\n"), DanglingSoft.Num());
	for (int32 Index = 0; Index < DanglingSoft.Num() && Index < 300; ++Index)
	{
		const FString Line = FString::Printf(TEXT("%s\n    -> %s\n    %s\n"),
			*DanglingSoft[Index].Package.ToString(),
			*DanglingSoft[Index].Dependency.ToString(),
			*DanglingSoft[Index].Note);
		Report += Line;
		AddInfo(FString::Printf(TEXT("[探针] 悬空软引用: %s -> %s"),
			*DanglingSoft[Index].Package.ToString(), *DanglingSoft[Index].Dependency.ToString()));
	}

	// ---- 4) 遍历器自检 ----
	//
	// headless 跑的时候 /Game 下**一个包都没加载**, 所以上面的 0 是「没有东西可扫」,
	// 不是「遍历器坏了」。用最宽的范围再跑一遍, 确认属性遍历机制本身是通的:
	// 只要 NumObjectsVisited > 0, 就说明 TObjectIterator + 属性反射这条路走得通。
	const FDoodleReferenceAudit::FSoftScanStats SelfCheck =
		FDoodleReferenceAudit::ScanSoftReferences(TEXT("/"), /*bDirtyPackagesOnly*/ false);

	AddInfo(FString::Printf(
		TEXT("[探针] 遍历器自检 (范围 /): 包 %d / 对象 %d / 软引用属性 %d / 命中根路径 %d"),
		SelfCheck.NumPackagesVisited, SelfCheck.NumObjectsVisited,
		SelfCheck.NumSoftPropertiesSeen, SelfCheck.NumSoftPathsSeen));

	TestTrue(TEXT("软引用遍历器必须能遍历到对象 (为 0 说明属性反射遍历机制失效)"),
		SelfCheck.NumObjectsVisited > 0);

	Report += FString::Printf(
		TEXT("\n--- 遍历器自检 (范围 /): 包 %d / 对象 %d / 软引用属性 %d / 命中根路径 %d ---\n"),
		SelfCheck.NumPackagesVisited, SelfCheck.NumObjectsVisited,
		SelfCheck.NumSoftPropertiesSeen, SelfCheck.NumSoftPathsSeen);

	const FString ReportFile = FDoodleReferenceAudit::WriteReportFile(TEXT("live_probe"), Report);
	AddInfo(FString::Printf(TEXT("[探针] 报告: %s"), *ReportFile));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
