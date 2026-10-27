#include "DoodleLightingBridge.h"

#include "DoodleLightingJson.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	const TCHAR* const GBridgeDirEnvVar = TEXT("DOODLE_AI_LIGHT_BRIDGE_DIR");

	const TCHAR* const GBridgeDirDefaultSub = TEXT("DoodleAiLight");

	const TCHAR* const GLaunchPlanPrefix = TEXT("launch_plan_");

	FString MakeLaunchPlanFileName(const FString& InSessionId, int32 InIteration)
	{
		return FString::Printf(TEXT("%s%s_it%03d.json"), GLaunchPlanPrefix, *InSessionId, InIteration);
	}
}

namespace DoodleLightingBridge
{
	FString GetBridgeDir()
	{
		FString L_Dir = FPlatformMisc::GetEnvironmentVariable(GBridgeDirEnvVar);
		L_Dir.TrimStartAndEndInline();
		if (L_Dir.IsEmpty())
		{
			L_Dir = FPaths::Combine(FPaths::ProjectSavedDir(), GBridgeDirDefaultSub);
		}
		return FPaths::ConvertRelativePathToFull(L_Dir);
	}

	FString MakeLaunchPlanPath(const FString& InSessionId, int32 InIteration)
	{
		return FPaths::Combine(GetBridgeDir(), MakeLaunchPlanFileName(InSessionId, InIteration));
	}

	bool WriteLaunchPlan(const FDoodleLaunchPlan& InPlan, FString& OutPath, FString& OutError)
	{
		OutPath = MakeLaunchPlanPath(InPlan.SessionId, InPlan.Iteration);

		const TSharedPtr<FJsonObject> L_Json = DoodleLightingJson::SerializeLaunchPlan(InPlan);
		if (!L_Json.IsValid())
		{
			OutError = TEXT("启动计划序列化失败");
			return false;
		}

		FString L_Text;
		const TSharedRef<TJsonWriter<>> L_Writer = TJsonWriterFactory<>::Create(&L_Text);
		if (!FJsonSerializer::Serialize(L_Json.ToSharedRef(), L_Writer))
		{
			OutError = TEXT("启动计划写 json 文本失败");
			return false;
		}

		const FString L_Dir = FPaths::GetPath(OutPath);
		if (!IFileManager::Get().MakeDirectory(*L_Dir, true))
		{
			OutError = FString::Printf(TEXT("建不了桥目录 %s"), *L_Dir);
			return false;
		}
		if (!FFileHelper::SaveStringToFile(L_Text, *OutPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			OutError = FString::Printf(TEXT("写不了启动计划 %s"), *OutPath);
			return false;
		}
		return true;
	}

	bool ReadLaunchPlan(const FString& InPath, FDoodleLaunchPlan& OutPlan, FString& OutError)
	{
		if (!IFileManager::Get().FileExists(*InPath))
		{
			OutError = FString::Printf(TEXT("启动计划不存在：%s"), *InPath);
			return false;
		}

		FString L_Text;
		if (!FFileHelper::LoadFileToString(L_Text, *InPath))
		{
			OutError = FString::Printf(TEXT("读不了启动计划 %s"), *InPath);
			return false;
		}

		TSharedPtr<FJsonObject> L_Root;
		const TSharedRef<TJsonReader<>> L_Reader = TJsonReaderFactory<>::Create(L_Text);
		if (!FJsonSerializer::Deserialize(L_Reader, L_Root) || !L_Root.IsValid())
		{
			OutError = FString::Printf(TEXT("启动计划不是合法 json：%s"), *InPath);
			return false;
		}
		return DoodleLightingJson::ParseLaunchPlan(L_Root, OutPlan, OutError);
	}

	bool DeleteLaunchPlan(const FString& InPath, FString& OutError)
	{
		if (!IFileManager::Get().FileExists(*InPath))
		{
			return true;
		}
		if (!IFileManager::Get().Delete(*InPath, false, true, true))
		{
			OutError = FString::Printf(TEXT("删不掉已消费的启动计划 %s"), *InPath);
			return false;
		}
		return true;
	}
}
