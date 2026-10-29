#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

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

	FText GetStatusText() const;

	TSharedPtr<SEditableTextBox> EmailBox;
	TSharedPtr<SEditableTextBox> PasswordBox;
	TSharedPtr<SMultiLineEditableTextBox> ResultBox;
	TSharedPtr<STextBlock> StatusText;
};
