#include "DoodleLightingReport.h"

#include "DoodleLightingJson.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString DoodleLightingReport::MakeReportPath(const FString& InOutDir)
{
	return FPaths::Combine(InOutDir, DoodleLightingProtocol::RenderReportFileName);
}

bool DoodleLightingReport::WriteReportFile(const FString& InOutDir, const FDoodleRoundReport& InReport, FString& OutError)
{
	if (InOutDir.IsEmpty())
	{
		OutError = TEXT("out_dir 为空，无法写渲染回传报告");
		return false;
	}

	IPlatformFile& LPlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!LPlatformFile.DirectoryExists(*InOutDir))
	{
		if (!LPlatformFile.CreateDirectoryTree(*InOutDir))
		{
			OutError = FString::Printf(TEXT("无法创建输出目录：%s"), *InOutDir);
			return false;
		}
	}

	FString LText;
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> LWriter =
	    TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&LText);
	if (!FJsonSerializer::Serialize(DoodleLightingJson::SerializeRoundReport(InReport).ToSharedRef(), LWriter))
	{
		OutError = TEXT("渲染回传报告序列化失败");
		return false;
	}

	const FString LPath = MakeReportPath(InOutDir);
	if (!FFileHelper::SaveStringToFile(LText, *LPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("渲染回传报告写入失败：%s"), *LPath);
		return false;
	}
	return true;
}

bool DoodleLightingReport::ReadReportFile(const FString& InFilePath, FDoodleRoundReport& OutReport, FString& OutError)
{
	if (!FPaths::FileExists(InFilePath))
	{
		OutError = FString::Printf(TEXT("渲染回传报告不存在：%s"), *InFilePath);
		return false;
	}

	FString LText;
	if (!FFileHelper::LoadFileToString(LText, *InFilePath))
	{
		OutError = FString::Printf(TEXT("渲染回传报告读取失败：%s"), *InFilePath);
		return false;
	}

	TSharedPtr<FJsonObject> LRoot;
	const TSharedRef<TJsonReader<>> LReader = TJsonReaderFactory<>::Create(LText);
	if (!FJsonSerializer::Deserialize(LReader, LRoot) || !LRoot.IsValid())
	{
		OutError = FString::Printf(TEXT("渲染回传报告不是合法 JSON：%s"), *InFilePath);
		return false;
	}
	return DoodleLightingJson::ParseRoundReport(LRoot, OutReport, OutError);
}
