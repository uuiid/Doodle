// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoodleLiveLinkWindow.h"
#include "DoodleLiveLink.h"

#include "Features/IModularFeatures.h"
#include "Framework/Application/SlateApplication.h"
#include "ILiveLinkClient.h"
#include "LiveLinkFaceSourceBlueprint.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** 配置输入行的标签样式。 */
	const FMargin ConfigLabelPadding(0.0f, 4.0f, 0.0f, 4.0f);
}

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

	// 关闭主窗口时退出程序。
	RootWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FDoodleLiveLinkWindow::OnWindowClosed));

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
			.Padding(0.0f, 24.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("DoodleLiveLink", "ConfigHeader", "Live Link Face 源配置"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(ConfigLabelPadding)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("DoodleLiveLink", "AddressLabel", "地址"))
					.MinDesiredWidth(80.0f)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(LiveLinkFaceAddress))
					.OnTextChanged_Lambda([this](const FText& InText)
						{
							LiveLinkFaceAddress = InText.ToString();
						})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(ConfigLabelPadding)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("DoodleLiveLink", "PortLabel", "端口"))
					.MinDesiredWidth(80.0f)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SNumericEntryBox<uint16>)
					.Value(LiveLinkFacePort)
					.OnValueChanged_Lambda([this](uint16 InValue)
						{
							LiveLinkFacePort = InValue;
						})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(ConfigLabelPadding)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("DoodleLiveLink", "SubjectNameLabel", "Subject 名"))
					.MinDesiredWidth(80.0f)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(LiveLinkFaceSubjectName))
					.OnTextChanged_Lambda([this](const FText& InText)
						{
							LiveLinkFaceSubjectName = InText.ToString();
						})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 24.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("DoodleLiveLink", "CreateLiveLinkFaceSource", "创建 Live Link Face 源"))
				.OnClicked_Lambda([this]() -> FReply
					{
						CreateLiveLinkFaceSource();
						return FReply::Handled();
					})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 24.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("DoodleLiveLink", "SourceListHeader", "源列表"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(12.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("DoodleLiveLink", "RefreshSourceList", "刷新"))
					.OnClicked_Lambda([this]() -> FReply
						{
							RefreshSourceList();
							return FReply::Handled();
						})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(SourceListBox, SVerticalBox)
				]
			]
		]
	);

	FSlateApplication::Get().AddWindow(RootWindow.ToSharedRef());

	RefreshSourceList();
}

void FDoodleLiveLinkWindow::CreateLiveLinkFaceSource()
{
	if (!bLiveLinkFaceSourceCreated)
	{
		bool bSucceeded = false;
		ULiveLinkFaceSourceBlueprint::CreateLiveLinkFaceSource(LiveLinkFaceSourceHandle, bSucceeded);
		if (!bSucceeded)
		{
			UE_LOG(LogDoodleLiveLink, Error, TEXT("创建 Live Link Face 源失败"));
			return;
		}

		bLiveLinkFaceSourceCreated = true;

		// 源已加入客户端，先刷新列表再尝试连接。
		RefreshSourceList();
	}

	// 使用当前界面配置连接（地址、端口、Subject 名称）；配置修改后点击可重新连接。
	bool bConnected = false;
	ULiveLinkFaceSourceBlueprint::Connect(LiveLinkFaceSourceHandle, LiveLinkFaceSubjectName, LiveLinkFaceAddress, bConnected, LiveLinkFacePort);
	RefreshSourceList();
	
	if (!bConnected)
	{
		UE_LOG(LogDoodleLiveLink, Error, TEXT("Live Link Face 源连接失败（%s:%d）"), *LiveLinkFaceAddress, LiveLinkFacePort);
		return;
	}

	UE_LOG(LogDoodleLiveLink, Log, TEXT("Live Link Face 源已连接（%s:%d, Subject=%s）"), *LiveLinkFaceAddress, LiveLinkFacePort, *LiveLinkFaceSubjectName);
}

void FDoodleLiveLinkWindow::RefreshSourceList()
{
	if (!SourceListBox.IsValid())
	{
		return;
	}

	SourceListBox->ClearChildren();

	ILiveLinkClient* LiveLinkClient = nullptr;
	if (IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		LiveLinkClient = &IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
	}

	if (!LiveLinkClient)
	{
		SourceListBox->AddSlot()
		             .AutoHeight()
		             .Padding(0.0f, 2.0f, 0.0f, 2.0f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("DoodleLiveLink", "NoLiveLinkClient", "Live Link 客户端不可用"))
		];
		return;
	}

	const TArray<FGuid> Sources = LiveLinkClient->GetSources();
	if (Sources.Num() == 0)
	{
		SourceListBox->AddSlot()
		             .AutoHeight()
		             .Padding(0.0f, 2.0f, 0.0f, 2.0f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("DoodleLiveLink", "NoSources", "（暂无源）"))
		];
		return;
	}

	for (const FGuid& SourceGuid : Sources)
	{
		const FText SourceType = LiveLinkClient->GetSourceType(SourceGuid);
		const FText MachineName = LiveLinkClient->GetSourceMachineName(SourceGuid);
		const FText Status = LiveLinkClient->GetSourceStatus(SourceGuid);

		SourceListBox->AddSlot()
		             .AutoHeight()
		             .Padding(0.0f, 2.0f, 0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(SourceType)
				.MinDesiredWidth(160.0f)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(MachineName)
				.MinDesiredWidth(160.0f)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(Status)
			]
		];
	}
}

void FDoodleLiveLinkWindow::OnWindowClosed(const TSharedRef<SWindow>& InWindow)
{
	RequestEngineExit(TEXT("DoodleLiveLink 主窗口已关闭"));
}
