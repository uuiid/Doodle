#include "SDoodleKitsuLoginPanel.h"

#include "DoodleKitsuClient.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	constexpr double AutoRefreshIntervalSeconds = 5.0;
}

void SDoodleKitsuLoginPanel::Construct(const FArguments& InArgs)
{
	SetCanTick(true);

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f)
		[
			SNew(STextBlock)
			.Text(this, &SDoodleKitsuLoginPanel::GetStatusText)
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
				SNew(SButton)
				.Text(FText::FromString(TEXT("刷新")))
				.OnClicked(this, &SDoodleKitsuLoginPanel::OnRefreshClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(16.0f, 0.0f, 0.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(AutoRefreshBox, SCheckBox)
				.IsChecked(ECheckBoxState::Unchecked)
				.OnCheckStateChanged(this, &SDoodleKitsuLoginPanel::OnAutoRefreshChanged)
				[
					SNew(STextBlock)
					.Text(this, &SDoodleKitsuLoginPanel::GetAutoRefreshText)
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(16.0f, 0.0f, 0.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return RefreshResult; })
			]
		]
	];
}

FReply SDoodleKitsuLoginPanel::OnRefreshClicked()
{
	if (bRefreshing)
	{
		return FReply::Handled();
	}

	bRefreshing = true;
	LastRefreshSeconds = FPlatformTime::Seconds();

	const DoodleKitsuClient::FDoodleIterationResult L_StartResult = DoodleKitsuClient::StartFetchCurrentIteration(
		[L_WeakPanel = TWeakPtr<SDoodleKitsuLoginPanel>(SharedThis(this))](DoodleKitsuClient::FDoodleIterationResult InResult)
		{
			const TSharedPtr<SDoodleKitsuLoginPanel> L_Panel = L_WeakPanel.Pin();
			if (!L_Panel.IsValid())
			{
				return;
			}
			L_Panel->bRefreshing = false;
			L_Panel->RefreshResult = InResult.HasValue()
										? FText::FromString(FString::Printf(
											  TEXT("第 %d 轮，候选 %d 个"), InResult.GetValue().Iteration, InResult.GetValue().Rigs.Num()
										  ))
										: FText::FromString(InResult.GetError());
		}
	);

	if (L_StartResult.HasError())
	{
		bRefreshing = false;
		RefreshResult = FText::FromString(L_StartResult.GetError());
	}

	return FReply::Handled();
}

void SDoodleKitsuLoginPanel::OnAutoRefreshChanged(ECheckBoxState InState)
{
	bAutoRefresh = InState == ECheckBoxState::Checked;
	LastRefreshSeconds = FPlatformTime::Seconds();
}

FText SDoodleKitsuLoginPanel::GetStatusText() const
{
	const TValueOrError<FString, FString> L_ServerUrl = DoodleKitsuClient::GetLocalServerUrl();
	if (L_ServerUrl.HasError())
	{
		return FText::FromString(FString::Printf(TEXT("本机服务: 读不到端口（%s）"), *L_ServerUrl.GetError()));
	}
	return FText::FromString(FString::Printf(TEXT("本机服务: %s"), *L_ServerUrl.GetValue()));
}

FText SDoodleKitsuLoginPanel::GetAutoRefreshText() const
{
	return FText::FromString(TEXT("自动刷新（每 5 秒）"));
}

void SDoodleKitsuLoginPanel::Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(InAllottedGeometry, InCurrentTime, InDeltaTime);

	if (!bAutoRefresh || bRefreshing)
	{
		return;
	}

	if (FPlatformTime::Seconds() - LastRefreshSeconds < AutoRefreshIntervalSeconds)
	{
		return;
	}

	OnRefreshClicked();
}
