#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"
#include "Templates/Function.h"
#include "Templates/ValueOrError.h"

namespace DoodleKitsuClient
{
	struct FDoodleLoginValue
	{
		FString UserId;
		FString UserName;
		FString Email;
	};

	using FDoodleLoginResult = TValueOrError<FDoodleLoginValue, FString>;
	using FDoodleLoginCallback = TFunction<void(FDoodleLoginResult)>;

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

	DOODLELIGHTINGEDITOR_API FString GetServerUrl();

	DOODLELIGHTINGEDITOR_API bool IsLoggedIn();

	DOODLELIGHTINGEDITOR_API FString MakeBearerAuthorization();

	DOODLELIGHTINGEDITOR_API FDoodleLoginResult StartLogin(
		const FString& InEmail, const FString& InPassword, FDoodleLoginCallback InOnComplete
	);

	DOODLELIGHTINGEDITOR_API FDoodleIterationResult StartFetchCurrentIteration(
		const FString& InSessionId, FDoodleIterationCallback InOnComplete
	);

	DOODLELIGHTINGEDITOR_API bool Logout();
}
