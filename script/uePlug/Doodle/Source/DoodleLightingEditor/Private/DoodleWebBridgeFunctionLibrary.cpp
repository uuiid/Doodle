#include "DoodleWebBridgeFunctionLibrary.h"

#include "DoodleLightingEditorLog.h"
#include "DoodleWebBridge.h"

#include "Misc/FileHelper.h"

FString UDoodleWebBridgeFunctionLibrary::SetAiLightIterationFromFile(const FString& InFilePath)
{
	if (InFilePath.IsEmpty())
	{
		const FString L_Error = TEXT("文件路径为空");
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 读取失败: %s"), *L_Error);
		return L_Error;
	}

	FString L_Json;
	if (!FFileHelper::LoadFileToString(L_Json, *InFilePath))
	{
		const FString L_Error = FString::Printf(TEXT("读取文件失败：%s"), *InFilePath);
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 读取失败: %s"), *L_Error);
		return L_Error;
	}

	return UDoodleWebBridge::Get()->SetAiLightIteration(L_Json);
}
