#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#include "DoodleKitsuClient.h"

class SEditableTextBox;
class SMultiLineEditableTextBox;
class STextBlock;

class SDoodleKitsuLoginPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDoodleKitsuLoginPanel)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnLoginClicked();
	FReply OnLogoutClicked();

	void ApplyLoginResult(DoodleKitsuClient::FDoodleLoginResult InResult);

	FText GetStatusText() const;
	bool IsLoggingIn() const;
	bool IsLoginButtonEnabled() const;

	TSharedPtr<SEditableTextBox> EmailBox;
	TSharedPtr<SEditableTextBox> PasswordBox;
	TSharedPtr<SMultiLineEditableTextBox> ResultBox;
	TSharedPtr<STextBlock> StatusText;
	bool bLoggingIn = false;
};
