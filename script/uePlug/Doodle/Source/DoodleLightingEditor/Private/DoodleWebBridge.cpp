#include "DoodleWebBridge.h"

#include "DoodleKitsuClient.h"
#include "DoodleLightingEditorLog.h"
#include "DoodleLightingRoundRunner.h"
#include "DoodleLightingSettings.h"

#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	FString GetCurrentMapPath()
	{
		const UWorld* L_World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (L_World == nullptr)
		{
			return FString();
		}
		return L_World->GetPathName();
	}
}

void UDoodleWebBridge::Logout()
{
	UDoodleLightingSettings* L_Settings = GetMutableDefault<UDoodleLightingSettings>();
	L_Settings->Cookies.Reset();
	L_Settings->SaveConfig();
}

FString UDoodleWebBridge::SetAiLightIteration(const FString& InIteration)
{
	TSharedPtr<FJsonObject> L_Root;
	const TSharedRef<TJsonReader<>> L_Reader = TJsonReaderFactory<>::Create(InIteration);
	if (!FJsonSerializer::Deserialize(L_Reader, L_Root) || !L_Root.IsValid())
	{
		const FString L_Error = FString::Printf(TEXT("不是合法 json 对象，长度 %d"), InIteration.Len());
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 解析失败: %s"), *L_Error);
		return L_Error;
	}

	const DoodleKitsuClient::FDoodleIterationResult L_Result = DoodleKitsuClient::ParseIterationBody(L_Root);
	if (L_Result.HasError())
	{
		const FString L_Error = FString::Printf(TEXT("轮次解析失败: %s"), *L_Result.GetError());
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 解析失败: %s"), *L_Error);
		return L_Error;
	}

	const DoodleKitsuClient::FDoodleLightingIterationValue& L_Value = L_Result.GetValue();

	const FString L_CurrentMap = GetCurrentMapPath();
	if (L_CurrentMap.IsEmpty())
	{
		const FString L_Error = TEXT("编辑器里没有打开的关卡，无法校验基线关卡");
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 校验失败: %s"), *L_Error);
		return L_Error;
	}

	if (!L_Value.BaselineLevel.Equals(L_CurrentMap, ESearchCase::IgnoreCase))
	{
		const FString L_Error = FString::Printf(
			TEXT("基线关卡不匹配：本轮要求 %s，当前打开的是 %s"),
			L_Value.BaselineLevel.IsEmpty() ? TEXT("<空>") : *L_Value.BaselineLevel, *L_CurrentMap
		);
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 校验失败: %s"), *L_Error);
		return L_Error;
	}

	UE_LOG(
		LogDoodleLightingEditor, Log,
		TEXT("ai light iteration 第 %d 轮，state=%s，会话 %s，镜头任务 %s，基线关卡 %s，渲染关卡 %s，候选 %d 个"),
		L_Value.Iteration, *L_Value.State, *L_Value.SessionId, *L_Value.ShotTaskId, *L_Value.BaselineLevel,
		*L_Value.RenderMap, L_Value.Rigs.Num()
	);

	if (L_Value.Rigs.IsEmpty())
	{
		const FString L_Error = TEXT("本轮没有候选灯光方案");
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 校验失败: %s"), *L_Error);
		return L_Error;
	}

	const FString L_OutDir = FPaths::Combine(
		FPaths::ProjectSavedDir(), TEXT("DoodleLighting"), L_Value.SessionId, FString::FromInt(L_Value.Iteration)
	);

	for (int32 L_Index = 0; L_Index < L_Value.Rigs.Num(); ++L_Index)
	{
		const FDoodleLightingRig& L_Rig = L_Value.Rigs[L_Index];
		UE_LOG(
			LogDoodleLightingEditor, Log, TEXT("  候选 %d：rig_id=%s，spec_id=%s，灯 %d 盏，op %d 条"), L_Index,
			*L_Rig.RigId, *L_Rig.SpecId, L_Rig.Lights.Num(), L_Rig.Ops.Num()
		);

		UDoodleLightingRoundRunner* L_Runner = NewObject<UDoodleLightingRoundRunner>(GetTransientPackage());
		L_Runner->AddToRoot();
		L_Runner->RoundFinished.AddWeakLambda(L_Runner, [L_Runner](const FDoodleRoundReport& InReport)
		{
			L_Runner->RemoveFromRoot();
		});

		const DoodleLightingExecutor::FRoundRenderResultOrError L_StartResult = L_Runner->StartRender(
			L_Value.RenderMap, L_Value.BaselineLevel, L_Value.MoviePipelineConfig, L_OutDir, L_Rig
		);
		if (L_StartResult.HasError())
		{
			L_Runner->RemoveFromRoot();
			const FString L_Error = FString::Printf(TEXT("候选 %s 起渲染失败: %s"), *L_Rig.CandidateId,
			                                        *L_StartResult.GetError());
			UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 起渲染失败: %s"), *L_Error);
			return L_Error;
		}
	}

	return FString();
}

UDoodleWebBridge* UDoodleWebBridge::Get()
{
	static UDoodleWebBridge* L_Bridge = []()
	{
		UDoodleWebBridge* L_New = NewObject<UDoodleWebBridge>(GetTransientPackage());
		L_New->AddToRoot();
		return L_New;
	}();
	return L_Bridge;
}
