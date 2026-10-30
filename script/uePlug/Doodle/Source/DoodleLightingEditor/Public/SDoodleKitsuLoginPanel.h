#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SDoodleKitsuLoginPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDoodleKitsuLoginPanel)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FText GetStatusText() const;
};
