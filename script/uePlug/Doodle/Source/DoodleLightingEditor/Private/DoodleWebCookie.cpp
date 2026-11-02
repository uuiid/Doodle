#include "DoodleWebCookie.h"

#include "DoodleLightingEditorLog.h"

#include "IWebBrowserCookieManager.h"
#include "IWebBrowserSingleton.h"
#include "WebBrowserModule.h"

namespace
{
	const TCHAR* const AccessTokenCookieName = TEXT("access_token_cookie");
	const TCHAR* const AccessTokenCookiePath = TEXT("/");
	constexpr int32 AccessTokenCookieLifetimeDays = 365;
}

namespace DoodleWebCookie
{
	TValueOrError<FString, FString> ResolveCookieOrigin(const FString& InServerUrl)
	{
		const FString L_Url = InServerUrl.TrimStartAndEnd();
		if (L_Url.IsEmpty())
		{
			return MakeError(TEXT("远程 URL 为空"));
		}

		const int32 L_SchemeEnd = L_Url.Find(TEXT("://"));
		if (L_SchemeEnd == INDEX_NONE)
		{
			return MakeError(TEXT("远程 URL 必须以 http:// 或 https:// 开头"));
		}

		const int32 L_HostBegin = L_SchemeEnd + 3;
		int32 L_HostEnd = L_Url.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, L_HostBegin);
		if (L_HostEnd == INDEX_NONE)
		{
			L_HostEnd = L_Url.Len();
		}

		const FString L_Origin = L_Url.Left(L_HostEnd);
		if (L_Origin.Len() <= L_HostBegin)
		{
			return MakeError(TEXT("远程 URL 里没有主机名"));
		}
		return MakeValue(L_Origin);
	}

	void SetAccessTokenCookie(const FString& InOriginUrl, const FString& InAccessToken, TFunction<void(bool)> InOnComplete)
	{
		const FString L_Token = InAccessToken.TrimStartAndEnd();
		if (InOriginUrl.IsEmpty() || L_Token.IsEmpty())
		{
			InOnComplete(false);
			return;
		}

		IWebBrowserModule* L_Module = FModuleManager::GetModulePtr<IWebBrowserModule>(TEXT("WebBrowser"));
		if (L_Module == nullptr || !L_Module->IsWebModuleAvailable())
		{
			UE_LOG(LogDoodleLightingEditor, Error, TEXT("浏览器模块不可用，写不进 cookie"));
			InOnComplete(false);
			return;
		}

		IWebBrowserSingleton* L_Singleton = L_Module->GetSingleton();
		if (L_Singleton == nullptr)
		{
			UE_LOG(LogDoodleLightingEditor, Error, TEXT("浏览器没有初始化，写不进 cookie"));
			InOnComplete(false);
			return;
		}

		const TSharedPtr<IWebBrowserCookieManager> L_CookieManager = L_Singleton->GetCookieManager();
		if (!L_CookieManager.IsValid())
		{
			UE_LOG(LogDoodleLightingEditor, Error, TEXT("拿不到 cookie 管理器，写不进 cookie"));
			InOnComplete(false);
			return;
		}

		FWebBrowserCookie L_Cookie;
		L_Cookie.Name = AccessTokenCookieName;
		L_Cookie.Value = L_Token;
		L_Cookie.Path = AccessTokenCookiePath;
		L_Cookie.bSecure = false;
		L_Cookie.bHttpOnly = true;
		L_Cookie.bHasExpires = true;
		L_Cookie.Expires = FDateTime::UtcNow() + FTimespan::FromDays(AccessTokenCookieLifetimeDays);

		L_CookieManager->SetCookie(
			InOriginUrl, L_Cookie,
			[L_OnComplete = MoveTemp(InOnComplete)](bool bInSuccess)
			{
				if (!bInSuccess)
				{
					UE_LOG(LogDoodleLightingEditor, Error, TEXT("浏览器拒绝写入 access_token_cookie"));
				}
				L_OnComplete(bInSuccess);
			}
		);
	}
}
