// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoodleLiveLinkWindow.h"
#include "DoodleLiveLink.h"

#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

FDoodleLiveLinkWindow::FDoodleLiveLinkWindow()
{
	if (!FSlateApplication::IsInitialized())
	{
		FSlateApplication::Create();
	}
}

FDoodleLiveLinkWindow::~FDoodleLiveLinkWindow()
{
	if (RootWindow.IsValid())
	{
		RootWindow->RequestDestroyWindow();
	}
}

void FDoodleLiveLinkWindow::CreateWindow()
{
	RootWindow = SNew(SWindow)
		.Title(NSLOCTEXT("DoodleLiveLink", "WindowTitle", "Doodle Live Link"))
		.ClientSize(FVector2D(900.0f, 600.0f))
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		.SizingRule(ESizingRule::UserSized);

	RootWindow->SetContent(
		SNew(SBorder)
		.Padding(24.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("DoodleLiveLink", "Title", "Doodle Live Link 中心"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("DoodleLiveLink", "Status", "WebSocket 服务端已启动，等待 Maya 客户端连接..."))
			]
		]
	);

	FSlateApplication::Get().AddWindow(RootWindow.ToSharedRef());
}
