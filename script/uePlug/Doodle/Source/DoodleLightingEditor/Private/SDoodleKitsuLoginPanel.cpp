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
				.OnClicked(this, &SDoodleKitsuLoginPanel::OnLoginClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("退出登录")))
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
	L_Settings->TryUpdateDefaultConfigFile();

	const DoodleKitsuClient::FDoodleLoginResult L_Result = DoodleKitsuClient::Login(L_Email, L_Password);

	if (PasswordBox.IsValid())
	{
		PasswordBox->SetText(FText::GetEmpty());
	}

	if (ResultBox.IsValid())
	{
		ResultBox->SetText(FText::FromString(
			L_Result.bSucceeded ? FString::Printf(TEXT("登录成功: %s (%s)"), *L_Result.UserName, *L_Result.Email) : L_Result.Error
		));
	}

	return FReply::Handled();
}

FReply SDoodleKitsuLoginPanel::OnLogoutClicked()
{
	DoodleKitsuClient::Logout();

	if (ResultBox.IsValid())
	{
		ResultBox->SetText(FText::FromString(TEXT("已退出登录")));
	}

	return FReply::Handled();
}

FText SDoodleKitsuLoginPanel::GetStatusText() const
{
	if (!DoodleKitsuClient::IsLoggedIn())
	{
		return FText::FromString(TEXT("未登录"));
	}

	const UDoodleLightingSettings* L_Settings = GetDefault<UDoodleLightingSettings>();
	return FText::FromString(FString::Printf(TEXT("已登录: %s"), *L_Settings->UserName));
}
