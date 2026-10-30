#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FSpawnTabArgs;
class SDockTab;
class SWebBrowser;

class SDoodleKitsuLoginTab : public SCompoundWidget
{
public:
	static const FName TabName;

	SLATE_BEGIN_ARGS(SDoodleKitsuLoginTab)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& InSpawnTabArgs);

private:
	FString GetConfiguredUrl() const;
	bool HasLoggedIn() const;
	bool CanSubmitLogin() const;
	TSharedRef<SWidget> BuildBrowser();
	TSharedRef<SWidget> BuildLoginForm();
	void LoadUrlAfterCookie();
	void OpenBrowserUrl();
	FReply OnOpenSettingsClicked();
	FReply OnLoginClicked();
	void OnLoginFinished(bool bInSuccess, const FString& InMessage);

	FText LoadError;
	FString LoadedUrl;
	TSharedPtr<SWebBrowser> WebBrowser;

	FString PendingUrl;
	FString PendingEmail;
	FString PendingPassword;
	FString LoginMessage;
	bool bLoginFailed = false;
	bool bLoginInFlight = false;
};
