#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"
#include "Dom/JsonObject.h"
#include "Templates/Function.h"
#include "Templates/ValueOrError.h"

namespace DoodleKitsuClient
{
	struct FDoodleLightingIterationValue
	{
		int32 Iteration = 0;
		FString State;
		FString RenderAt;
		FString FeedbackText;
		FString SessionId;
		FString ShotTaskId;
		FString BaselineLevel;
		FString RenderMap;
		FString MoviePipelineConfig;
		TArray<FDoodleLightingRig> Rigs;
	};

	using FDoodleIterationResult = TValueOrError<FDoodleLightingIterationValue, FString>;

	struct FDoodleLoginValue
	{
		FString AccessToken;
		FString RefreshToken;
		FString Email;
		FString DisplayName;
	};

	using FDoodleLoginResult = TValueOrError<FDoodleLoginValue, FString>;
	using FDoodleLoginCallback = TFunction<void(FDoodleLoginResult)>;

	DOODLELIGHTINGEDITOR_API FDoodleIterationResult ParseIterationBody(const TSharedPtr<FJsonObject>& InRoot);

	DOODLELIGHTINGEDITOR_API FDoodleLoginResult StartLogin(
		const FString& InServerUrl, const FString& InEmail, const FString& InPassword, FDoodleLoginCallback InOnComplete
	);
}
