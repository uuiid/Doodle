#include "DoodleWebBridge.h"

#include "DoodleLightingSettings.h"

void UDoodleWebBridge::Logout()
{
	UDoodleLightingSettings* L_Settings = GetMutableDefault<UDoodleLightingSettings>();
	L_Settings->Cookies.Reset();
	L_Settings->SaveConfig();
}

void UDoodleWebBridge::SetAiLightIteration(const FString& InIteration) {}

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
