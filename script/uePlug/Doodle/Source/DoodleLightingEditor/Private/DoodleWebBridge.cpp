#include "DoodleWebBridge.h"

#include "DoodleLightingSettings.h"

void UDoodleWebBridge::SetCookies(const FString& InCookies)
{
	SetCookiesFromNative(InCookies);
}

void UDoodleWebBridge::SetCookiesFromNative(const FString& InCookies)
{
	UDoodleLightingSettings* L_Settings = GetMutableDefault<UDoodleLightingSettings>();
	L_Settings->Cookies = InCookies;
	L_Settings->SaveConfig();
}

FString UDoodleWebBridge::GetCookies()
{
	return GetDefault<UDoodleLightingSettings>()->Cookies;
}

UDoodleWebBridge* UDoodleWebBridge::Get()
{
	static UDoodleWebBridge* L_Bridge = []()
	{
		UDoodleWebBridge* L_New = NewObject<UDoodleWebBridge>(GetTransientPackage());
		L_New->AddToRoot();
		return L_New;
	}();
	return L_Bridge;
}
