// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Doodle/Organize/DoodleAssetPathUtils.h"

#include "Misc/PackageName.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleOrganizeSuffixNameTest,
	"Doodle.Organize.PathUtils.SuffixName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleOrganizeSuffixNameTest::RunTest(const FString& Parameters)
{
	FString Out;

	// ---- 加后缀 ----
	TestTrue(TEXT("T_rock + low 应成功"), DoodleOrganize::MakeSuffixedName(TEXT("T_rock"), TEXT("low"), Out));
	TestEqual(TEXT("T_rock + low"), Out, FString(TEXT("T_rock_low")));

	TestFalse(TEXT("T_rock_low + low 应幂等跳过"),
		DoodleOrganize::MakeSuffixedName(TEXT("T_rock_low"), TEXT("low"), Out));

	TestFalse(TEXT("空后缀应失败"),
		DoodleOrganize::MakeSuffixedName(TEXT("T_rock"), TEXT("   "), Out));

	TestFalse(TEXT("空名字应失败"),
		DoodleOrganize::MakeSuffixedName(TEXT(""), TEXT("low"), Out));

	// 后缀首尾空格会被裁掉
	TestTrue(TEXT("带空格的后缀应被裁剪"), DoodleOrganize::MakeSuffixedName(TEXT("T_rock"), TEXT("  low  "), Out));
	TestEqual(TEXT("带空格的后缀结果"), Out, FString(TEXT("T_rock_low")));

	// ---- 去后缀 (新语义: 只剥离真正匹配的后缀) ----
	TestTrue(TEXT("T_rock_low - low 应成功"), DoodleOrganize::MakeUnsuffixedName(TEXT("T_rock_low"), TEXT("low"), Out));
	TestEqual(TEXT("T_rock_low - low"), Out, FString(TEXT("T_rock")));

	// 修正点: 不再把 T_rock 变成 T
	TestFalse(TEXT("T_rock - low 不应改动"),
		DoodleOrganize::MakeUnsuffixedName(TEXT("T_rock"), TEXT("low"), Out));

	TestFalse(TEXT("T_rock_01 - low 不应改动"),
		DoodleOrganize::MakeUnsuffixedName(TEXT("T_rock_01"), TEXT("low"), Out));

	TestFalse(TEXT("空后缀去后缀应失败"),
		DoodleOrganize::MakeUnsuffixedName(TEXT("T_rock_low"), TEXT(""), Out));

	// ---- 旧行为逃生开关 ----
	TestTrue(TEXT("legacy: T_rock_low 应成功"), DoodleOrganize::MakeUnsuffixedNameLegacy(TEXT("T_rock_low"), Out));
	TestEqual(TEXT("legacy: T_rock_low"), Out, FString(TEXT("T_rock")));

	TestTrue(TEXT("legacy: T_rock_01 应成功"), DoodleOrganize::MakeUnsuffixedNameLegacy(TEXT("T_rock_01"), Out));
	TestEqual(TEXT("legacy: T_rock_01"), Out, FString(TEXT("T_rock")));

	// 原实现会把 T_rock 变成 T, 这里收紧为不处理
	TestFalse(TEXT("legacy: 没有下划线时不应改动"), DoodleOrganize::MakeUnsuffixedNameLegacy(TEXT("Trock"), Out));
	TestFalse(TEXT("legacy: 下划线开头时不应改动"), DoodleOrganize::MakeUnsuffixedNameLegacy(TEXT("_rock"), Out));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleOrganizePackagePathTest,
	"Doodle.Organize.PathUtils.PackagePath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleOrganizePackagePathTest::RunTest(const FString& Parameters)
{
	// ---- StripObjectPath ----
	TestEqual(TEXT("去掉对象路径"),
		DoodleOrganize::StripObjectPath(TEXT("/Game/A/B.B")), FString(TEXT("/Game/A/B")));
	TestEqual(TEXT("去掉子对象路径"),
		DoodleOrganize::StripObjectPath(TEXT("/Game/A/B.B:Sub")), FString(TEXT("/Game/A/B")));
	TestEqual(TEXT("纯包名保持不变"),
		DoodleOrganize::StripObjectPath(TEXT("/Game/A/B")), FString(TEXT("/Game/A/B")));

	// ---- CombinePackagePath ----
	TestEqual(TEXT("拼接 (无尾斜杠)"),
		DoodleOrganize::CombinePackagePath(TEXT("/Game/A"), TEXT("B")), FString(TEXT("/Game/A/B")));
	TestEqual(TEXT("拼接 (有尾斜杠)"),
		DoodleOrganize::CombinePackagePath(TEXT("/Game/A/"), TEXT("B")), FString(TEXT("/Game/A/B")));

	// ---- IsUnderPackagePath ----
	TestTrue(TEXT("子路径"), DoodleOrganize::IsUnderPackagePath(TEXT("/Game/A/B"), TEXT("/Game/A")));
	TestTrue(TEXT("自身"), DoodleOrganize::IsUnderPackagePath(TEXT("/Game/A"), TEXT("/Game/A")));
	TestTrue(TEXT("自身 (尾斜杠)"), DoodleOrganize::IsUnderPackagePath(TEXT("/Game/A"), TEXT("/Game/A/")));
	TestFalse(TEXT("前缀相同但不是子路径"),
		DoodleOrganize::IsUnderPackagePath(TEXT("/Game/AB"), TEXT("/Game/A")));

	// ---- IsInternalPackagePath ----
	TestTrue(TEXT("__ExternalActors__"),
		DoodleOrganize::IsInternalPackagePath(TEXT("/Game/__ExternalActors__/Map/AB/CD")));
	TestTrue(TEXT("__ExternalObjects__"),
		DoodleOrganize::IsInternalPackagePath(TEXT("/Game/__ExternalObjects__/Map/AB/CD")));
	TestFalse(TEXT("普通包"),
		DoodleOrganize::IsInternalPackagePath(TEXT("/Game/Map/Level")));

	// ---- IsNonAssetPackagePath (悬空引用扫描的假阳性过滤器) ----
	TestTrue(TEXT("/Script/Engine 不是资产"),
		DoodleOrganize::IsNonAssetPackagePath(TEXT("/Script/Engine")));
	TestTrue(TEXT("/Script/InterchangeEngine 不是资产"),
		DoodleOrganize::IsNonAssetPackagePath(TEXT("/Script/InterchangeEngine")));
	TestTrue(TEXT("/Temp 不是资产"), DoodleOrganize::IsNonAssetPackagePath(TEXT("/Temp/Foo")));
	TestTrue(TEXT("空串按非资产处理"), DoodleOrganize::IsNonAssetPackagePath(TEXT("")));
	TestFalse(TEXT("/Game 是资产"),
		DoodleOrganize::IsNonAssetPackagePath(TEXT("/Game/CZ721/Meshs/12/qs")));
	TestFalse(TEXT("/Engine 内容是资产"),
		DoodleOrganize::IsNonAssetPackagePath(TEXT("/Engine/BasicShapes/Cube")));
	TestFalse(TEXT("/Scripts 不是 /Script"),
		DoodleOrganize::IsNonAssetPackagePath(TEXT("/Scripts/Foo")));
	TestFalse(TEXT("/Game/Character 是资产"),
		DoodleOrganize::IsNonAssetPackagePath(TEXT("/Game/Character/MoMingYiFu/Meshs/YiFu")));

	// ---- TryConvertFilenameToPackagePath 往返 ----
	{
		const FString OriginalPackage = TEXT("/Game/DoodleOrganizeTest/Foo");
		const FString Filename = FPackageName::LongPackageNameToFilename(OriginalPackage, TEXT(".uasset"));
		FString Converted;
		TestTrue(TEXT("文件名应能转回包名"), DoodleOrganize::TryConvertFilenameToPackagePath(Filename, Converted));
		TestEqual(TEXT("往返结果"), Converted, OriginalPackage);
	}

	// ---- TryMakeUniqueAssetPath: 空闲路径原样返回 ----
	{
		const FString FreeFolder = TEXT("/Game/DoodleOrganizeTest_DefinitelyMissing");
		FString OutFolder;
		FString OutName;
		TestTrue(TEXT("空闲路径应成功"),
			DoodleOrganize::TryMakeUniqueAssetPath(FreeFolder, TEXT("NewAsset"), OutFolder, OutName));
		TestEqual(TEXT("空闲路径的目录"), OutFolder, FreeFolder);
		TestEqual(TEXT("空闲路径的名字"), OutName, FString(TEXT("NewAsset")));
	}

	// ---- TryMakeUniqueAssetPath: 非法入参 ----
	{
		FString OutFolder;
		FString OutName;
		TestFalse(TEXT("空目录应失败"),
			DoodleOrganize::TryMakeUniqueAssetPath(TEXT(""), TEXT("NewAsset"), OutFolder, OutName));
		TestFalse(TEXT("空名字应失败"),
			DoodleOrganize::TryMakeUniqueAssetPath(TEXT("/Game"), TEXT(""), OutFolder, OutName));
	}

	// ---- GetGameRootPath ----
	TestEqual(TEXT("Game 根路径"), DoodleOrganize::GetGameRootPath(), FString(TEXT("/Game")));

	return true;
}

// ---------------------------------------------------------------------------
// 「整理后引用丢失」修复的核心逻辑
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleOrganizeRetargetPathTest,
	"Doodle.Organize.PathUtils.Retarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleOrganizeRetargetPathTest::RunTest(const FString& Parameters)
{
	TMap<FString, FString> MoveMap;
	MoveMap.Add(TEXT("/Game/Old/T_rock"), TEXT("/Game/Tex/T_rock"));
	MoveMap.Add(TEXT("/Game/Old/SM_Cube"), TEXT("/Game/Mesh/SM_Cube"));

	// 命中: 对象路径后缀必须保留
	TestEqual(TEXT("对象路径"),
		DoodleOrganize::RetargetObjectPath(TEXT("/Game/Old/T_rock.T_rock"), MoveMap),
		FString(TEXT("/Game/Tex/T_rock.T_rock")));

	// 命中: 纯包名
	TestEqual(TEXT("纯包名"),
		DoodleOrganize::RetargetObjectPath(TEXT("/Game/Old/T_rock"), MoveMap),
		FString(TEXT("/Game/Tex/T_rock")));

	// 命中: 子对象路径 (例如蓝图里的组件)
	TestEqual(TEXT("子对象路径"),
		DoodleOrganize::RetargetObjectPath(TEXT("/Game/Old/SM_Cube.SM_Cube:StaticMeshComponent0"), MoveMap),
		FString(TEXT("/Game/Mesh/SM_Cube.SM_Cube:StaticMeshComponent0")));

	// 未命中: 不是被移动的包
	TestTrue(TEXT("未命中应返回空串"),
		DoodleOrganize::RetargetObjectPath(TEXT("/Game/Other/T_rock.T_rock"), MoveMap).IsEmpty());

	// 未命中: 前缀相同但不是同一个包 (避免把 /Game/Old/T_rock_2 误改)
	TestTrue(TEXT("同前缀不同包不应命中"),
		DoodleOrganize::RetargetObjectPath(TEXT("/Game/Old/T_rock_2.T_rock_2"), MoveMap).IsEmpty());

	// 未命中: 目标其实在别的挂载点
	TestTrue(TEXT("引擎内容不应命中"),
		DoodleOrganize::RetargetObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube"), MoveMap).IsEmpty());

	// 幂等: 已经在新路径上的引用不会再被改 (引擎已修好的那些)
	TestTrue(TEXT("已在新路径上应返回空串"),
		DoodleOrganize::RetargetObjectPath(TEXT("/Game/Tex/T_rock.T_rock"), MoveMap).IsEmpty());

	// 边界
	TestTrue(TEXT("空路径"), DoodleOrganize::RetargetObjectPath(TEXT(""), MoveMap).IsEmpty());
	TestTrue(TEXT("空移动表"),
		DoodleOrganize::RetargetObjectPath(TEXT("/Game/Old/T_rock.T_rock"), TMap<FString, FString>()).IsEmpty());

	// 旧包 == 新包 (原地不动) 不应产生改写
	{
		TMap<FString, FString> InPlace;
		InPlace.Add(TEXT("/Game/A/X"), TEXT("/Game/A/X"));
		TestTrue(TEXT("原地不动不应改写"),
			DoodleOrganize::RetargetObjectPath(TEXT("/Game/A/X.X"), InPlace).IsEmpty());
	}

	return true;
}

/**
 * 批次内目标预订的回归测试。
 *
 * 真实事故: 224 个请求里有两个同名资产 (例如 /Game/CZ721/Meshs/10/ysMSK_fg01 与
 * /Game/Character/test_1/Texture/ysMSK_fg01) 都被分到 /Game/Character/test_2/Texture/ysMSK_fg01。
 * 注册表和磁盘在分配时都还没变, 所以 TryMakeUniqueAssetPath 认为目标空闲;
 * 引擎随后让这 7 个撞车的失败, 并且因为"有任一失败就整批返回 Failure"
 * (AssetRenameManager.cpp:553), 上层误判整批 224 个都失败 —— 连带把三道引用保护全部跳过。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleOrganizeBatchReservationTest,
	"Doodle.Organize.PathUtils.BatchReservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleOrganizeBatchReservationTest::RunTest(const FString& Parameters)
{
	// 用一个磁盘/注册表上都不存在的目录, 保证结果只取决于预订集合
	const FString Folder = TEXT("/Game/DoodleOrganizeTest_NotOnDisk/Texture");
	const FString Desired = TEXT("ysMSK_fg01");
	const FString DesiredPackage = DoodleOrganize::CombinePackagePath(Folder, Desired);

	// ---- 没有预订时: 目标空闲 ----
	{
		TestTrue(TEXT("未预订且不存在时应空闲"),
			DoodleOrganize::IsAssetPathFreeForBatch(DesiredPackage, nullptr));

		TSet<FString> Empty;
		TestTrue(TEXT("空预订集合时应空闲"),
			DoodleOrganize::IsAssetPathFreeForBatch(DesiredPackage, &Empty));
	}

	// ---- 已被本批别的资产预订: 不再空闲 ----
	{
		TSet<FString> Reserved;
		Reserved.Add(DesiredPackage);

		TestFalse(TEXT("已被本批预订时不应空闲"),
			DoodleOrganize::IsAssetPathFreeForBatch(DesiredPackage, &Reserved));

		// 预订别的路径不影响本路径
		TSet<FString> OtherReserved;
		OtherReserved.Add(DoodleOrganize::CombinePackagePath(Folder, TEXT("其它资产")));
		TestTrue(TEXT("预订别的路径不应影响本路径"),
			DoodleOrganize::IsAssetPathFreeForBatch(DesiredPackage, &OtherReserved));
	}

	// ---- 核心回归: 同一批里两次分配同名资产必须得到两个不同的目标 ----
	{
		TSet<FString> Reserved;

		FString Folder1, Name1;
		TestTrue(TEXT("第一个应分配到原名"),
			DoodleOrganize::TryMakeUniqueAssetPath(Folder, Desired, Folder1, Name1, &Reserved));
		TestEqual(TEXT("第一个名字"), Name1, Desired);
		Reserved.Add(DoodleOrganize::CombinePackagePath(Folder1, Name1));

		FString Folder2, Name2;
		TestTrue(TEXT("第二个应能分配到别的名字"),
			DoodleOrganize::TryMakeUniqueAssetPath(Folder, Desired, Folder2, Name2, &Reserved));
		TestNotEqual(TEXT("第二个名字必须与第一个不同"), Name2, Name1);

		const FString Package1 = DoodleOrganize::CombinePackagePath(Folder1, Name1);
		const FString Package2 = DoodleOrganize::CombinePackagePath(Folder2, Name2);
		TestNotEqual(TEXT("两个目标包必须不同"), Package2, Package1);

		// 第三个也应该拿到新名字 (能持续分配)
		Reserved.Add(Package2);
		FString Folder3, Name3;
		TestTrue(TEXT("第三个应能分配到别的名字"),
			DoodleOrganize::TryMakeUniqueAssetPath(Folder, Desired, Folder3, Name3, &Reserved));
		TestNotEqual(TEXT("第三个名字必须与前两个不同"), Name3, Name1);
		TestNotEqual(TEXT("第三个名字必须与前两个不同 (2)"), Name3, Name2);
	}

	// ---- 不传预订集合时保持旧行为 (向后兼容) ----
	{
		FString Folder1, Name1;
		TestTrue(TEXT("不传预订集合时仍可用"),
			DoodleOrganize::TryMakeUniqueAssetPath(Folder, Desired, Folder1, Name1));
		TestEqual(TEXT("不传预订集合时给原名"), Name1, Desired);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
