#include "DoodleWebBridge.h"

#include "DoodleKitsuClient.h"
#include "DoodleLightingSettings.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UDoodleWebBridge::Logout()
{
	UDoodleLightingSettings* L_Settings = GetMutableDefault<UDoodleLightingSettings>();
	L_Settings->Cookies.Reset();
	L_Settings->SaveConfig();
}

void UDoodleWebBridge::SetAiLightIteration(const FString& InIteration)
{
	TSharedPtr<FJsonObject> L_Root;
	const TSharedRef<TJsonReader<>> L_Reader = TJsonReaderFactory<>::Create(InIteration);
	if (!FJsonSerializer::Deserialize(L_Reader, L_Root) || !L_Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DoodleWebBridge] ai light iteration 不是合法 json 对象，长度 %d"), InIteration.Len());
		return;
	}

	const DoodleKitsuClient::FDoodleIterationResult L_Result = DoodleKitsuClient::ParseIterationBody(L_Root);
	if (L_Result.HasError())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DoodleWebBridge] ai light iteration 解析失败: %s"), *L_Result.GetError());
		return;
	}

	const DoodleKitsuClient::FDoodleLightingIterationValue& L_Value = L_Result.GetValue();
	UE_LOG(
		LogTemp, Log,
		TEXT("[DoodleWebBridge] ai light iteration 第 %d 轮，state=%s，会话 %s，镜头任务 %s，基线关卡 %s，候选 %d 个"),
		L_Value.Iteration, *L_Value.State, *L_Value.SessionId, *L_Value.ShotTaskId, *L_Value.BaselineLevel,
		L_Value.Rigs.Num()
	);

	for (int32 L_Index = 0; L_Index < L_Value.Rigs.Num(); ++L_Index)
	{
		const FDoodleLightingRig& L_Rig = L_Value.Rigs[L_Index];
		UE_LOG(
			LogTemp, Log, TEXT("[DoodleWebBridge]   候选 %d：rig_id=%s，spec_id=%s，灯 %d 盏，op %d 条"), L_Index,
			*L_Rig.RigId, *L_Rig.SpecId, L_Rig.Lights.Num(), L_Rig.Ops.Num()
		);
	}
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
