#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FSpawnTabArgs;
class SDockTab;

class SDoodleKitsuLoginTab : public SCompoundWidget
{
public:
	static const FName TabName;

	SLATE_BEGIN_ARGS(SDoodleKitsuLoginTab)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& InSpawnTabArgs);
};
