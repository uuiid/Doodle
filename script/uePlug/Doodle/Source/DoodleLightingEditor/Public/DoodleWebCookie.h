#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "Templates/ValueOrError.h"

namespace DoodleWebCookie
{
	DOODLELIGHTINGEDITOR_API TValueOrError<FString, FString> ResolveCookieOrigin(const FString& InServerUrl);

	DOODLELIGHTINGEDITOR_API void SetAccessTokenCookie(
		const FString& InOriginUrl, const FString& InAccessToken, TFunction<void(bool)> InOnComplete
	);
}
