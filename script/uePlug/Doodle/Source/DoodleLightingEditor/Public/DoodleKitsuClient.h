#pragma once

#include "CoreMinimal.h"

namespace DoodleKitsuClient
{
	struct DOODLELIGHTINGEDITOR_API FDoodleLoginResult
	{
		bool bSucceeded = false;
		FString Error;
		FString UserId;
		FString UserName;
		FString Email;
	};

	DOODLELIGHTINGEDITOR_API FString GetServerUrl();

	DOODLELIGHTINGEDITOR_API bool IsLoggedIn();

	DOODLELIGHTINGEDITOR_API FString MakeBearerAuthorization();

	DOODLELIGHTINGEDITOR_API FDoodleLoginResult Login(const FString& InEmail, const FString& InPassword);

	DOODLELIGHTINGEDITOR_API void Logout();

	DOODLELIGHTINGEDITOR_API bool LoginIntoActiveSession(const FString& InEmail, const FString& InPassword);
}
