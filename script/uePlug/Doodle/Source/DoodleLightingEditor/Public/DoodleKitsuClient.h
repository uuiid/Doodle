#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"
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
		TArray<FDoodleLightingRig> Rigs;
	};

	using FDoodleIterationResult = TValueOrError<FDoodleLightingIterationValue, FString>;
	using FDoodleIterationCallback = TFunction<void(FDoodleIterationResult)>;

	struct FDoodleLoginValue
	{
		FString AccessToken;
		FString RefreshToken;
		FString Email;
		FString DisplayName;
	};

	using FDoodleLoginResult = TValueOrError<FDoodleLoginValue, FString>;
	using FDoodleLoginCallback = TFunction<void(FDoodleLoginResult)>;

	DOODLELIGHTINGEDITOR_API TValueOrError<FString, FString> GetLocalServerUrl();

	DOODLELIGHTINGEDITOR_API FDoodleIterationResult StartFetchCurrentIteration(FDoodleIterationCallback InOnComplete);

	DOODLELIGHTINGEDITOR_API FDoodleLoginResult StartLogin(
		const FString& InServerUrl, const FString& InEmail, const FString& InPassword, FDoodleLoginCallback InOnComplete
	);
}
