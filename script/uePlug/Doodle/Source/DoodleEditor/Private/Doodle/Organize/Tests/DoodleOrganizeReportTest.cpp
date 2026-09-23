// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Doodle/Organize/DoodleOrganizeTypes.h"

namespace
{
	FDoodleOrganizeReport MakeBaseReport()
	{
		FDoodleOrganizeReport Report;
		Report.Operation = TEXT("UnitTest");
		Report.NumRequested = 3;
		Report.NumMoved = 3;
		Report.Finalize();
		return Report;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleOrganizeReportSummaryTest,
	"Doodle.Organize.Report.Summary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleOrganizeReportSummaryTest::RunTest(const FString& Parameters)
{
	// ---- 基础字段总是显示 ----
	{
		const FDoodleOrganizeReport Report = MakeBaseReport();
		const FString Summary = Report.ToSummaryText().ToString();

		TestTrue(TEXT("应含操作名"), Summary.Contains(TEXT("UnitTest")));
		TestTrue(TEXT("应含移动数"), Summary.Contains(TEXT("移动 3")));
	}

	// ---- 新字段: 没有发生时不要出现在报告条里 (否则太长) ----
	{
		const FDoodleOrganizeReport Report = MakeBaseReport();
		const FString Summary = Report.ToSummaryText().ToString();

		TestFalse(TEXT("没保存脏包时不应显示"), Summary.Contains(TEXT("保存脏包")));
		TestFalse(TEXT("没补修时不应显示"), Summary.Contains(TEXT("补修软引用")));
		TestFalse(TEXT("没跑第二趟时不应显示"), Summary.Contains(TEXT("已加载全部软引用者")));
		TestFalse(TEXT("没删重定向器时不应显示"), Summary.Contains(TEXT("删除重定向器")));
	}

	// ---- 新字段: 发生了就要显示 ----
	{
		FDoodleOrganizeReport Report = MakeBaseReport();
		Report.NumDirtyPackagesSaved = 2;
		Report.NumRetargetedSoftReferences = 5;
		Report.bRanLoadAllPackagesPass = true;
		Report.NumRedirectorsDeleted = 4;

		const FString Summary = Report.ToSummaryText().ToString();

		TestTrue(TEXT("应显示保存脏包数"), Summary.Contains(TEXT("保存脏包 2")));
		TestTrue(TEXT("应显示补修软引用数"), Summary.Contains(TEXT("补修软引用 5")));
		TestTrue(TEXT("应显示第二趟"), Summary.Contains(TEXT("已加载全部软引用者")));
		TestTrue(TEXT("应显示删除重定向器数"), Summary.Contains(TEXT("删除重定向器 4")));
	}

	// ---- 取消 / 非法输入走短格式 (不能把半截数字塞进去) ----
	{
		FDoodleOrganizeReport Report;
		Report.Operation = TEXT("Cancelled");
		Report.Result = EDoodleOrganizeResult::Cancelled;
		Report.NumDirtyPackagesSaved = 9;

		const FString Summary = Report.ToSummaryText().ToString();
		TestTrue(TEXT("取消时应含操作名"), Summary.Contains(TEXT("Cancelled")));
		TestFalse(TEXT("取消时不应展开计数"), Summary.Contains(TEXT("保存脏包")));
	}

	// ---- 详细文本仍然把警告/失败列出来 ----
	{
		FDoodleOrganizeReport Report = MakeBaseReport();
		Report.AddWarning(TEXT("第二趟已加载所有软引用者 (含地图) 并修复引用: 7 个包。"));

		const FString Detailed = Report.ToDetailedText();
		TestTrue(TEXT("详细文本应含警告"), Detailed.Contains(TEXT("警告")));
		TestTrue(TEXT("详细文本应含警告内容"), Detailed.Contains(TEXT("第二趟")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
