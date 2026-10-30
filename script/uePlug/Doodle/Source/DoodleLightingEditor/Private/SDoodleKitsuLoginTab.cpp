#include "SDoodleKitsuLoginTab.h"

#include "DoodleLightingSettings.h"

#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "SWebBrowser.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"

const FName SDoodleKitsuLoginTab::TabName{ TEXT("DoodleKitsuLoginTab") };

namespace
{
	constexpr int32 BrowserSlotIndex = 0;
	constexpr int32 HintSlotIndex = 1;
}

void SDoodleKitsuLoginTab::Construct(const FArguments& InArgs)
{
	SetCanTick(true);

	ChildSlot
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex_Lambda([this]() { return HasConfiguredUrl() ? BrowserSlotIndex : HintSlotIndex; })

			+ SWidgetSwitcher::Slot()
			[
				BuildBrowser()
			]

			+ SWidgetSwitcher::Slot()
			[
				BuildMissingUrlHint()
			]
		];

	LoadedUrl = GetConfiguredUrl();
	if (!LoadedUrl.IsEmpty())
	{
		WebBrowser->LoadURL(LoadedUrl);
	}
}

TSharedRef<SWidget> SDoodleKitsuLoginTab::BuildBrowser()
{
	return SAssignNew(WebBrowser, SWebBrowser)
		.InitialURL(GetConfiguredUrl())
		.ShowControls(false)
		.ShowAddressBar(false)
		.ShowInitialThrobber(true)
		.OnLoadError_Lambda([this]() { LoadError = FText::FromString(TEXT("网页加载失败，请检查设置里的服务器 URL")); })
		.OnLoadCompleted_Lambda([this]() { LoadError = FText::GetEmpty(); });
}

TSharedRef<SWidget> SDoodleKitsuLoginTab::BuildMissingUrlHint()
{
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(16.0f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(TEXT("还没有设置服务器 URL\n请先在 设置 → 插件 → Doodle Lighting 里填写“服务器 URL”")))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(16.0f, 0.0f, 16.0f, 16.0f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("打开设置")))
				.OnClicked(this, &SDoodleKitsuLoginTab::OnOpenSettingsClicked)
			]
		];
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

bool SDoodleKitsuLoginTab::HasConfiguredUrl() const
{
	return !GetConfiguredUrl().IsEmpty();
}

void SDoodleKitsuLoginTab::Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(InAllottedGeometry, InCurrentTime, InDeltaTime);

	const FString L_Url = GetConfiguredUrl();
	if (L_Url == LoadedUrl)
	{
		return;
	}

	LoadedUrl = L_Url;
	LoadError = FText::GetEmpty();
	if (!L_Url.IsEmpty() && WebBrowser.IsValid())
	{
		WebBrowser->LoadURL(L_Url);
	}
}

TSharedRef<SDockTab> SDoodleKitsuLoginTab::OnSpawnAction(const FSpawnTabArgs& InSpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SDoodleKitsuLoginTab)
		];
}
