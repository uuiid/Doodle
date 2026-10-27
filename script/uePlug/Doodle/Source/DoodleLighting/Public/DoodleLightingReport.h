#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"

namespace DoodleLightingReport
{
	DOODLELIGHTING_API FString MakeReportPath(const FString& InOutDir);

	DOODLELIGHTING_API bool WriteReportFile(const FString& InOutDir, const FDoodleRoundReport& InReport, FString& OutError);

	DOODLELIGHTING_API bool ReadReportFile(const FString& InFilePath, FDoodleRoundReport& OutReport, FString& OutError);
}
