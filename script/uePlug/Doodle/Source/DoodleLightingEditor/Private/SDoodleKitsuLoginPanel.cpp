#include "SDoodleKitsuLoginPanel.h"

#include "DoodleKitsuClient.h"
#include "DoodleLightingSettings.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

void SDoodleKitsuLoginPanel::Construct(const FArguments& InArgs)
{
	UDoodleLightingSettings* L_Settings = GetMutableDefault<UDoodleLightingSettings>();

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("服务器地址")))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(8.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([]() { return FText::FromString(DoodleKitsuClient::GetServerUrl()); })
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("在 Project Settings -> Plugins -> Doodle Lighting 修改")))
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("邮箱")))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(8.0f, 0.0f)
			[
				SAssignNew(EmailBox, SEditableTextBox)
				.Text(FText::FromString(L_Settings->Email))
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("密码")))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(8.0f, 0.0f)
			[
				SAssignNew(PasswordBox, SEditableTextBox)
				.IsPassword(true)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("登录")))
				.IsEnabled(this, &SDoodleKitsuLoginPanel::IsLoginButtonEnabled)
				.OnClicked(this, &SDoodleKitsuLoginPanel::OnLoginClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("退出登录")))
				.IsEnabled(this, &SDoodleKitsuLoginPanel::IsLoginButtonEnabled)
				.OnClicked(this, &SDoodleKitsuLoginPanel::OnLogoutClicked)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(8.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(StatusText, STextBlock)
				.Text(this, &SDoodleKitsuLoginPanel::GetStatusText)
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(8.0f)
		[
			SNew(SBox)
			.MinDesiredHeight(120.0f)
			[
				SNew(SScrollBox)

				+ SScrollBox::Slot()
				[
					SAssignNew(ResultBox, SMultiLineEditableTextBox)
					.IsReadOnly(true)
				]
			]
		]
	];
}

FReply SDoodleKitsuLoginPanel::OnLoginClicked()
{
	const FString L_Email = EmailBox.IsValid() ? EmailBox->GetText().ToString() : FString();
	const FString L_Password = PasswordBox.IsValid() ? PasswordBox->GetText().ToString() : FString();

	UDoodleLightingSettings* L_Settings = GetMutableDefault<UDoodleLightingSettings>();
	L_Settings->Email = L_Email;
	const bool bEmailSaved = L_Settings->TryUpdateDefaultConfigFile();

	if (!bEmailSaved)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DoodleKitsu] 登录邮箱没能写入配置文件"));
	}

	if (PasswordBox.IsValid())
	{
		PasswordBox->SetText(FText::GetEmpty());
	}

	if (ResultBox.IsValid())
	{
		ResultBox->SetText(FText::GetEmpty());
	}

	bLoggingIn = true;

	const DoodleKitsuClient::FDoodleLoginResult L_StartResult = DoodleKitsuClient::StartLogin(
		L_Email, L_Password,
		[L_WeakPanel = TWeakPtr<SDoodleKitsuLoginPanel>(SharedThis(this))](DoodleKitsuClient::FDoodleLoginResult InResult)
		{
			if (const TSharedPtr<SDoodleKitsuLoginPanel> L_Panel = L_WeakPanel.Pin())
			{
				L_Panel->ApplyLoginResult(MoveTemp(InResult));
			}
		}
	);

	if (L_StartResult.HasError())
	{
		bLoggingIn = false;
		if (ResultBox.IsValid())
		{
			ResultBox->SetText(FText::FromString(L_StartResult.GetError()));
		}
	}

	return FReply::Handled();
}

void SDoodleKitsuLoginPanel::ApplyLoginResult(DoodleKitsuClient::FDoodleLoginResult InResult)
{
	bLoggingIn = false;

	if (!ResultBox.IsValid())
	{
		return;
	}

	if (InResult.HasValue())
	{
		ResultBox->SetText(FText::FromString(FString::Printf(
			TEXT("登录成功: %s (%s)"), *InResult.GetValue().UserName, *InResult.GetValue().Email
		)));
		return;
	}

	ResultBox->SetText(FText::FromString(InResult.GetError()));
}

FReply SDoodleKitsuLoginPanel::OnLogoutClicked()
{
	const bool bLoggedOut = DoodleKitsuClient::Logout();

	if (ResultBox.IsValid())
	{
		ResultBox->SetText(FText::FromString(bLoggedOut ? TEXT("已退出登录") : TEXT("已退出登录，但登录凭据没能从配置文件清除")));
	}

	return FReply::Handled();
}

FText SDoodleKitsuLoginPanel::GetStatusText() const
{
	if (bLoggingIn)
	{
		return FText::FromString(TEXT("登录中…"));
	}

	if (!DoodleKitsuClient::IsLoggedIn())
	{
		return FText::FromString(TEXT("未登录"));
	}

	const UDoodleLightingSettings* L_Settings = GetDefault<UDoodleLightingSettings>();
	return FText::FromString(FString::Printf(TEXT("已登录: %s"), *L_Settings->UserName));
}

bool SDoodleKitsuLoginPanel::IsLoggingIn() const
{
	return bLoggingIn;
}

bool SDoodleKitsuLoginPanel::IsLoginButtonEnabled() const
{
	return !bLoggingIn;
}
