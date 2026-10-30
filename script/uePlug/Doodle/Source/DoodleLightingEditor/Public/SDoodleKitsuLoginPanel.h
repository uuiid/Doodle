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
	FReply OnFetchIterationClicked();

	void ApplyLoginResult(DoodleKitsuClient::FDoodleLoginResult InResult);
	void ApplyIterationResult(DoodleKitsuClient::FDoodleIterationResult InResult);

	FString FormatIterationSummary(const DoodleKitsuClient::FDoodleLightingIterationValue& InValue) const;

	FText GetStatusText() const;
	bool IsLoggingIn() const;
	bool IsLoginButtonEnabled() const;
	bool IsFetchButtonEnabled() const;

	TSharedPtr<SEditableTextBox> EmailBox;
	TSharedPtr<SEditableTextBox> PasswordBox;
	TSharedPtr<SEditableTextBox> SessionIdBox;
	TSharedPtr<SMultiLineEditableTextBox> ResultBox;
	TSharedPtr<STextBlock> StatusText;
	bool bLoggingIn = false;
	bool bFetchingIteration = false;
};
