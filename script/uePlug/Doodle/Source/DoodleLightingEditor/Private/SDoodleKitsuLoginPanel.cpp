#include "SDoodleKitsuLoginPanel.h"

#include "DoodleKitsuClient.h"

#include "Widgets/Text/STextBlock.h"

void SDoodleKitsuLoginPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(STextBlock)
		.Text(this, &SDoodleKitsuLoginPanel::GetStatusText)
	];
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
