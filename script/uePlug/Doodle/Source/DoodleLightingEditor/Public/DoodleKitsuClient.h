#pragma once

#include "CoreMinimal.h"
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

	DOODLELIGHTINGEDITOR_API FString GetServerUrl();

	DOODLELIGHTINGEDITOR_API bool IsLoggedIn();

	DOODLELIGHTINGEDITOR_API FString MakeBearerAuthorization();

	DOODLELIGHTINGEDITOR_API FDoodleLoginResult StartLogin(
		const FString& InEmail, const FString& InPassword, FDoodleLoginCallback InOnComplete
	);

	DOODLELIGHTINGEDITOR_API bool Logout();
}
