#include "DoodleWebBridge.h"

#include "DoodleKitsuClient.h"
#include "DoodleLightingSettings.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const TCHAR* const c_SampleBaselineLevel = TEXT(
		"/Game/Shot/ep0388/LQ388_sc028/Import_DH/LQ_EP388_SC028_DH.LQ_EP388_SC028_DH"
	);
	const TCHAR* const c_SampleSessionId = TEXT("4f1a2b3c-5d6e-4f70-8192-a3b4c5d6e7f8");
	const TCHAR* const c_SampleShotTaskId = TEXT("9c8b7a65-4321-4fed-8cba-0987654321fe");
	const TCHAR* const c_SampleRenderMap = TEXT("/Game/Shot/ep0388/LQ388_sc028/Import_DH/sc028_DH.sc028_DH");
	const TCHAR* const c_SampleMoviePipelineConfig = TEXT("/Game/DoodleLighting/MRQ_AiLight.MRQ_AiLight");
	const TCHAR* const c_SampleCandidate0 = TEXT("0d1e2f3a-4b5c-4d6e-8f90-a1b2c3d4e5f6");
	const TCHAR* const c_SampleCandidate1 = TEXT("7a6b5c4d-3e2f-4019-8b7a-6c5d4e3f2a1b");
	const TCHAR* const c_NotJson = TEXT("这不是 json");
	const TCHAR* const c_NotObject = TEXT("[1, 2, 3]");

	FString ResolveSampleIterationPath()
	{
		const FString L_BuildPath = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(FPaths::ProjectDir(), TEXT("../../build/ai_light_iteration.json"))
		);
		if (IFileManager::Get().FileExists(*L_BuildPath))
		{
			return L_BuildPath;
		}
		return FString(TEXT("E:/Doodle/build/ai_light_iteration.json"));
	}

	FString LoadSampleIteration()
	{
		FString L_Content;
		FFileHelper::LoadFileToString(L_Content, *ResolveSampleIterationPath());
		return L_Content;
	}

	DoodleKitsuClient::FDoodleIterationResult ParseIterationJson(const FString& InJson)
	{
		TSharedPtr<FJsonObject> L_Root;
		const TSharedRef<TJsonReader<>> L_Reader = TJsonReaderFactory<>::Create(InJson);
		if (!FJsonSerializer::Deserialize(L_Reader, L_Root) || !L_Root.IsValid())
		{
			return MakeError(FString(TEXT("不是合法 json 对象")));
		}
		return DoodleKitsuClient::ParseIterationBody(L_Root);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleWebBridgeSampleIterationTest,
	"Doodle.Lighting.WebBridge.SampleIteration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleWebBridgeSampleIterationTest::RunTest(const FString& Parameters)
{
	const FString L_Sample = LoadSampleIteration();
	TestFalse(TEXT("能读到 ai_light_iteration.json"), L_Sample.IsEmpty());
	if (L_Sample.IsEmpty())
	{
		AddError(FString::Printf(TEXT("读不到样例文件: %s"), *ResolveSampleIterationPath()));
		return false;
	}

	const DoodleKitsuClient::FDoodleIterationResult L_Result = ParseIterationJson(L_Sample);
	TestFalse(TEXT("样例能解析成轮次"), L_Result.HasError());
	if (L_Result.HasError())
	{
		AddError(FString::Printf(TEXT("解析失败: %s"), *L_Result.GetError()));
		return false;
	}

	const DoodleKitsuClient::FDoodleLightingIterationValue& L_Value = L_Result.GetValue();
	TestEqual(TEXT("轮次号"), L_Value.Iteration, 1);
	TestEqual(TEXT("状态"), L_Value.State, FString(TEXT("generated")));
	TestEqual(TEXT("会话 id"), L_Value.SessionId, FString(c_SampleSessionId));
	TestEqual(TEXT("镜头任务 id"), L_Value.ShotTaskId, FString(c_SampleShotTaskId));
	TestEqual(TEXT("基线关卡"), L_Value.BaselineLevel, FString(c_SampleBaselineLevel));
	TestEqual(TEXT("渲染关卡"), L_Value.RenderMap, FString(c_SampleRenderMap));
	TestEqual(TEXT("渲染配置"), L_Value.MoviePipelineConfig, FString(c_SampleMoviePipelineConfig));
	TestEqual(TEXT("候选数量"), L_Value.Rigs.Num(), 2);
	if (L_Value.Rigs.Num() != 2)
	{
		return false;
	}

	TestEqual(TEXT("第 0 个候选 id"), L_Value.Rigs[0].CandidateId, FString(c_SampleCandidate0));
	TestEqual(TEXT("第 0 个候选灯数"), L_Value.Rigs[0].Lights.Num(), 3);
	TestEqual(TEXT("第 1 个候选 id"), L_Value.Rigs[1].CandidateId, FString(c_SampleCandidate1));
	TestEqual(TEXT("第 1 个候选灯数"), L_Value.Rigs[1].Lights.Num(), 4);
	TestTrue(TEXT("第 1 个候选带 ops"), L_Value.Rigs[1].Ops.Num() > 0);
	if (L_Value.Rigs[0].Lights.Num() > 0)
	{
		TestEqual(TEXT("第 0 个候选第一盏灯 id"), L_Value.Rigs[0].Lights[0].Id, FString(TEXT("key_hero")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleWebBridgeIterationRejectTest,
	"Doodle.Lighting.WebBridge.IterationReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleWebBridgeIterationRejectTest::RunTest(const FString& Parameters)
{
	TSharedPtr<FJsonObject> L_Root;
	const TSharedRef<TJsonReader<>> L_Reader = TJsonReaderFactory<>::Create(FString(c_NotJson));
	TestFalse(TEXT("纯文本不是合法 json"), FJsonSerializer::Deserialize(L_Reader, L_Root) && L_Root.IsValid());

	L_Root.Reset();
	const TSharedRef<TJsonReader<>> L_ArrayReader = TJsonReaderFactory<>::Create(FString(c_NotObject));
	TestFalse(TEXT("裸数组不是 json 对象"), FJsonSerializer::Deserialize(L_ArrayReader, L_Root) && L_Root.IsValid());

	TestTrue(TEXT("空指针解析轮次失败"), DoodleKitsuClient::ParseIterationBody(nullptr).HasError());

	const TSharedPtr<FJsonObject> L_EmptyObject = MakeShared<FJsonObject>();
	TestTrue(TEXT("空对象解析轮次失败"), DoodleKitsuClient::ParseIterationBody(L_EmptyObject).HasError());

	const TSharedPtr<FJsonObject> L_NoState = MakeShared<FJsonObject>();
	L_NoState->SetNumberField(TEXT("iteration"), 1);
	TestTrue(TEXT("缺 state 解析轮次失败"), DoodleKitsuClient::ParseIterationBody(L_NoState).HasError());

	const TSharedPtr<FJsonObject> L_NoRigs = MakeShared<FJsonObject>();
	L_NoRigs->SetNumberField(TEXT("iteration"), 1);
	L_NoRigs->SetStringField(TEXT("state"), TEXT("generated"));
	const DoodleKitsuClient::FDoodleIterationResult L_NoRigsResult = DoodleKitsuClient::ParseIterationBody(L_NoRigs);
	TestFalse(TEXT("缺 rigs 也能解析出轮次"), L_NoRigsResult.HasError());
	if (L_NoRigsResult.HasValue())
	{
		TestEqual(TEXT("缺 rigs 时候选为空"), L_NoRigsResult.GetValue().Rigs.Num(), 0);
	}

	const TSharedPtr<FJsonObject> L_NoLights = MakeShared<FJsonObject>();
	L_NoLights->SetNumberField(TEXT("iteration"), 1);
	L_NoLights->SetStringField(TEXT("state"), TEXT("generated"));
	const TSharedPtr<FJsonObject> L_RigSet = MakeShared<FJsonObject>();
	const TSharedPtr<FJsonObject> L_Rig = MakeShared<FJsonObject>();
	L_Rig->SetStringField(TEXT("candidate_id"), TEXT("0d1e2f3a-4b5c-4d6e-8f90-a1b2c3d4e5f6"));
	TArray<TSharedPtr<FJsonValue>> L_RigArray;
	L_RigArray.Add(MakeShared<FJsonValueObject>(L_Rig));
	L_RigSet->SetArrayField(TEXT("rigs"), L_RigArray);
	L_NoLights->SetObjectField(TEXT("rigs"), L_RigSet);
	TestTrue(TEXT("候选没有灯时解析失败"), DoodleKitsuClient::ParseIterationBody(L_NoLights).HasError());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleWebBridgeLogoutTest,
	"Doodle.Lighting.WebBridge.Logout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleWebBridgeLogoutTest::RunTest(const FString& Parameters)
{
	UDoodleWebBridge* L_Bridge = UDoodleWebBridge::Get();
	TestNotNull(TEXT("能拿到 web bridge"), L_Bridge);
	TestTrue(TEXT("Get 每次都返回同一个实例"), L_Bridge == UDoodleWebBridge::Get());
	if (L_Bridge == nullptr)
	{
		return false;
	}

	UDoodleLightingSettings* L_Settings = GetMutableDefault<UDoodleLightingSettings>();
	TestNotNull(TEXT("能拿到设置对象"), L_Settings);
	if (L_Settings == nullptr)
	{
		return false;
	}

	const FString L_Backup = L_Settings->Cookies;
	L_Settings->Cookies = TEXT("doodle-cookie-for-test");

	L_Bridge->Logout();
	TestTrue(TEXT("Logout 清空了 Cookies"), L_Settings->Cookies.IsEmpty());

	L_Settings->Cookies = L_Backup;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoodleWebBridgeSetIterationTest,
	"Doodle.Lighting.WebBridge.SetIteration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoodleWebBridgeSetIterationTest::RunTest(const FString& Parameters)
{
	UDoodleWebBridge* L_Bridge = UDoodleWebBridge::Get();
	TestNotNull(TEXT("能拿到 web bridge"), L_Bridge);
	if (L_Bridge == nullptr)
	{
		return false;
	}

	const FString L_NotJsonError = L_Bridge->SetAiLightIteration(FString(c_NotJson));
	TestTrue(TEXT("非 json 返回非空错误"), !L_NotJsonError.IsEmpty());
	TestTrue(TEXT("非 json 的错误说明了原因"), L_NotJsonError.Contains(TEXT("不是合法 json 对象")));

	const FString L_NotObjectError = L_Bridge->SetAiLightIteration(FString(c_NotObject));
	TestTrue(TEXT("裸数组返回非空错误"), !L_NotObjectError.IsEmpty());

	const FString L_EmptyJsonError = L_Bridge->SetAiLightIteration(TEXT("{}"));
	TestTrue(TEXT("空对象返回非空错误"), !L_EmptyJsonError.IsEmpty());
	TestTrue(TEXT("空对象的错误来自轮次解析"), L_EmptyJsonError.Contains(TEXT("轮次解析失败")));

	const FString L_Sample = LoadSampleIteration();
	TestFalse(TEXT("能读到 ai_light_iteration.json"), L_Sample.IsEmpty());
	if (L_Sample.IsEmpty())
	{
		return false;
	}

	const FString L_SampleError = L_Bridge->SetAiLightIteration(L_Sample);
	TestTrue(TEXT("样例在无匹配关卡时返回非空错误"), !L_SampleError.IsEmpty());
	TestTrue(
		TEXT("错误里点名渲染关卡"),
		L_SampleError.Contains(TEXT("渲染关卡")) || L_SampleError.Contains(TEXT("没有打开的关卡"))
	);

	return true;
}

#endif
