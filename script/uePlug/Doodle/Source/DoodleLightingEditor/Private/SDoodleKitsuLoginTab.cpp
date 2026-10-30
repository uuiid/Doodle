#include "SDoodleKitsuLoginTab.h"

#include "DoodleKitsuClient.h"
#include "DoodleLightingSettings.h"
#include "DoodleWebBridge.h"
#include "DoodleWebCookie.h"

#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "SWebBrowser.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"

const FName SDoodleKitsuLoginTab::TabName{ TEXT("DoodleKitsuLoginTab") };

namespace
{
	constexpr int32 BrowserSlotIndex = 0;
	constexpr int32 LoginSlotIndex = 1;
}

void SDoodleKitsuLoginTab::Construct(const FArguments& InArgs)
{
	const UDoodleLightingSettings* L_Settings = GetDefault<UDoodleLightingSettings>();
	PendingUrl = L_Settings->LocalServerUrl;
	PendingUrl.TrimStartAndEndInline();
	PendingEmail = L_Settings->Account;
	PendingEmail.TrimStartAndEndInline();

	ChildSlot
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex_Lambda([this]() { return HasLoggedIn() ? BrowserSlotIndex : LoginSlotIndex; })

			+ SWidgetSwitcher::Slot()
			[
				BuildBrowser()
			]

			+ SWidgetSwitcher::Slot()
			[
				BuildLoginForm()
			]
		];

	LoadedUrl.Reset();
	if (HasLoggedIn())
	{
		LoadUrlAfterCookie();
	}
}

void SDoodleKitsuLoginTab::LoadUrlAfterCookie()
{
	const TValueOrError<FString, FString> L_Origin = DoodleWebCookie::ResolveCookieOrigin(GetConfiguredUrl());
	if (L_Origin.HasError())
	{
		LoadError = FText::FromString(L_Origin.GetError());
		return;
	}

	const FString L_Token = GetDefault<UDoodleLightingSettings>()->Cookies.TrimStartAndEnd();
	DoodleWebCookie::SetAccessTokenCookie(
		L_Origin.GetValue(), L_Token,
		[WeakThis = TWeakPtr<SDoodleKitsuLoginTab>(SharedThis(this))](bool bInWritten)
		{
			const TSharedPtr<SDoodleKitsuLoginTab> L_This = WeakThis.Pin();
			if (!L_This.IsValid() || !bInWritten)
			{
				return;
			}
			L_This->OpenBrowserUrl();
		}
	);
}

void SDoodleKitsuLoginTab::OpenBrowserUrl()
{
	LoadedUrl = GetConfiguredUrl();
	LoadError = FText::GetEmpty();
	if (WebBrowser.IsValid() && !LoadedUrl.IsEmpty())
	{
		WebBrowser->LoadURL(LoadedUrl);
	}
}

TSharedRef<SWidget> SDoodleKitsuLoginTab::BuildBrowser()
{
	TSharedRef<SWidget> L_BrowserWidget = SAssignNew(WebBrowser, SWebBrowser)
		.ShowControls(false)
		.ShowAddressBar(false)
		.ShowInitialThrobber(true)
		.OnLoadError_Lambda([this]() { LoadError = FText::FromString(TEXT("网页加载失败，请检查设置里的服务器 URL")); })
		.OnLoadCompleted_Lambda([this]() { LoadError = FText::GetEmpty(); });

	WebBrowser->BindUObject(TEXT("doodle"), UDoodleWebBridge::Get(), true);

	return L_BrowserWidget;
}

TSharedRef<SWidget> SDoodleKitsuLoginTab::BuildLoginForm()
{
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(24.0f)
		[
			SNew(SBox)
			.WidthOverride(420.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 16.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Doodle 登录")))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("远程 URL")))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(PendingUrl))
					.HintText(FText::FromString(TEXT("http://192.168.1.10")))
					.OnTextChanged_Lambda([this](const FText& InText) { PendingUrl = InText.ToString(); })
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("邮箱")))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(PendingEmail))
					.OnTextChanged_Lambda([this](const FText& InText) { PendingEmail = InText.ToString(); })
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("密码")))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 12.0f)
				[
					SNew(SEditableTextBox)
					.IsPassword(true)
					.OnTextChanged_Lambda([this](const FText& InText) { PendingPassword = InText.ToString(); })
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 12.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("登录")))
					.IsEnabled_Lambda([this]() { return CanSubmitLogin(); })
					.OnClicked(this, &SDoodleKitsuLoginTab::OnLoginClicked)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 8.0f)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
					.Text_Lambda([this]() { return FText::FromString(LoginMessage); })
					.ColorAndOpacity_Lambda([this]()
					{
						return bLoginFailed ? FSlateColor(FLinearColor(1.0f, 0.35f, 0.35f))
						                     : FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f));
					})
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("打开设置")))
					.OnClicked(this, &SDoodleKitsuLoginTab::OnOpenSettingsClicked)
				]
			]
		];
}

FReply SDoodleKitsuLoginTab::OnLoginClicked()
{
	if (!CanSubmitLogin())
	{
		return FReply::Handled();
	}

	bLoginInFlight = true;
	bLoginFailed = false;
	LoginMessage = TEXT("登录中…");

	UDoodleLightingSettings* L_Settings = GetMutableDefault<UDoodleLightingSettings>();
	L_Settings->LocalServerUrl = PendingUrl.TrimStartAndEnd();
	L_Settings->Account = PendingEmail.TrimStartAndEnd();
	L_Settings->SaveConfig();

	DoodleKitsuClient::FDoodleLoginResult L_Result = DoodleKitsuClient::StartLogin(
		PendingUrl, PendingEmail, PendingPassword,
		[WeakThis = TWeakPtr<SDoodleKitsuLoginTab>(SharedThis(this))](DoodleKitsuClient::FDoodleLoginResult InResult)
		{
			const TSharedPtr<SDoodleKitsuLoginTab> L_This = WeakThis.Pin();
			if (!L_This.IsValid())
			{
				return;
			}
			if (InResult.HasValue())
			{
				L_This->OnLoginFinished(true, FString());
			}
			else
			{
				L_This->OnLoginFinished(false, InResult.GetError());
			}
		}
	);

	if (L_Result.HasError())
	{
		OnLoginFinished(false, L_Result.GetError());
	}

	return FReply::Handled();
}

void SDoodleKitsuLoginTab::OnLoginFinished(bool bInSuccess, const FString& InMessage)
{
	bLoginInFlight = false;
	if (!bInSuccess)
	{
		bLoginFailed = true;
		LoginMessage = InMessage;
		return;
	}

	PendingPassword.Reset();
	bLoginFailed = false;
	LoginMessage.Reset();
	LoadError = FText::GetEmpty();

	LoadUrlAfterCookie();
}

FReply SDoodleKitsuLoginTab::OnOpenSettingsClicked()
{
	if (ISettingsModule* L_SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		const UDoodleLightingSettings* L_Settings = GetDefault<UDoodleLightingSettings>();
		L_SettingsModule->ShowViewer(
			L_Settings->GetContainerName(), L_Settings->GetCategoryName(), L_Settings->GetSectionName()
		);
	}
	return FReply::Handled();
}

FString SDoodleKitsuLoginTab::GetConfiguredUrl() const
{
	FString L_Url = GetDefault<UDoodleLightingSettings>()->LocalServerUrl;
	L_Url.TrimStartAndEndInline();
	return L_Url;
}

bool SDoodleKitsuLoginTab::HasLoggedIn() const
{
	return !GetConfiguredUrl().IsEmpty() && !GetDefault<UDoodleLightingSettings>()->Cookies.TrimStartAndEnd().IsEmpty();
}

bool SDoodleKitsuLoginTab::CanSubmitLogin() const
{
	if (bLoginInFlight)
	{
		return false;
	}
	return !PendingUrl.TrimStartAndEnd().IsEmpty() && !PendingEmail.TrimStartAndEnd().IsEmpty() &&
		   !PendingPassword.IsEmpty();
}

TSharedRef<SDockTab> SDoodleKitsuLoginTab::OnSpawnAction(const FSpawnTabArgs& InSpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SDoodleKitsuLoginTab)
		];
}
