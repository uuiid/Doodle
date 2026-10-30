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

	virtual void Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	FString GetConfiguredUrl() const;
	bool HasConfiguredUrl() const;
	TSharedRef<SWidget> BuildBrowser();
	TSharedRef<SWidget> BuildMissingUrlHint();
	FReply OnOpenSettingsClicked();

	FText LoadError;
	FString LoadedUrl;
	TSharedPtr<SWebBrowser> WebBrowser;
};
