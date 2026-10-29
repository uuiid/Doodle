#pragma once

#include "CoreMinimal.h"
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

	DOODLELIGHTINGEDITOR_API FString GetServerUrl();

	DOODLELIGHTINGEDITOR_API bool IsLoggedIn();

	DOODLELIGHTINGEDITOR_API FString MakeBearerAuthorization();

	DOODLELIGHTINGEDITOR_API FDoodleLoginResult Login(const FString& InEmail, const FString& InPassword);

	DOODLELIGHTINGEDITOR_API bool Logout();
}
