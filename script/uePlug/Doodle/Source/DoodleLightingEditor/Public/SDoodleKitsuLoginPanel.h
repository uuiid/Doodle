#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SCheckBox;

class SDoodleKitsuLoginPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDoodleKitsuLoginPanel)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnRefreshClicked();

	void OnAutoRefreshChanged(ECheckBoxState InState);

	FText GetStatusText() const;
	FText GetAutoRefreshText() const;

	void Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	TSharedPtr<SCheckBox> AutoRefreshBox;
	bool bAutoRefresh = false;
	bool bRefreshing = false;
	double LastRefreshSeconds = 0.0;
	FText RefreshResult;
};
