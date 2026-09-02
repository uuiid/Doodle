// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoodleLiveLinkWindow.h"
#include "DoodleLiveLink.h"
#include "DoodleLiveLinkForwarder.h"
#include "DoodleLiveLinkWebSocketServer.h"

#include "Features/IModularFeatures.h"
#include "Framework/Application/SlateApplication.h"
#include "ILiveLinkClient.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

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

	WebSocketServer = MakeUnique<FDoodleLiveLinkWebSocketServer>();
	Forwarder = MakeUnique<FDoodleLiveLinkForwarder>();
	Forwarder->SetFrameSink([this](const TArray<uint8>& InPayload)
	{
		if (WebSocketServer)
		{
			WebSocketServer->Broadcast(InPayload);
		}
	});
}

FDoodleLiveLinkWindow::~FDoodleLiveLinkWindow()
{
	Forwarder.Reset();
	WebSocketServer.Reset();

	// 关闭引擎阶段 Slate 可能已销毁，RequestDestroyWindow 会访问 FSlateApplication，需先判断是否仍可用。
	if (RootWindow.IsValid() && FSlateApplication::IsInitialized())
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
				SNew(STextBlock)
				.Text(NSLOCTEXT("DoodleLiveLink", "WebSocketHeader", "WebSocket 服务器"))
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
					.Text(NSLOCTEXT("DoodleLiveLink", "WebSocketPortLabel", "监听端口"))
					.MinDesiredWidth(80.0f)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SNumericEntryBox<uint16>)
					.Value(WebSocketServerPort)
					.OnValueChanged_Lambda([this](uint16 InValue)
						{
							if (InValue != WebSocketServerPort)
							{
								WebSocketServerPort = InValue;
								if (WebSocketServer)
								{
									WebSocketServer->Restart(WebSocketServerPort);
								}
							}
						})
				]
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
			.FillHeight(1.0f)
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SAssignNew(SourceListView, SListView<TSharedPtr<FSourceListItem>>)
				.ListItemsSource(&SourceListItems)
				.OnGenerateRow(this, &FDoodleLiveLinkWindow::OnGenerateSourceRow)
			]
		]
	);

	FSlateApplication::Get().AddWindow(RootWindow.ToSharedRef());

	RefreshSourceList();

	// 窗口创建时启动 WebSocket 服务器。
	if (WebSocketServer)
	{
		WebSocketServer->Start(WebSocketServerPort);
	}
}

void FDoodleLiveLinkWindow::CreateLiveLinkFaceSource()
{
	if (!Forwarder)
	{
		return;
	}

	Forwarder->ConnectSource(LiveLinkFaceAddress, LiveLinkFacePort, LiveLinkFaceSubjectName);
	RefreshSourceList();
}

void FDoodleLiveLinkWindow::RefreshSourceList()
{
	SourceListItems.Reset();

	ILiveLinkClient* LiveLinkClient = nullptr;
	if (IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		LiveLinkClient = &IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
	}

	if (!LiveLinkClient)
	{
		TSharedPtr<FSourceListItem> Item = MakeShared<FSourceListItem>();
		Item->Type = TEXT("Live Link 客户端不可用");
		SourceListItems.Add(MoveTemp(Item));
	}
	else
	{
		const TArray<FGuid> Sources = LiveLinkClient->GetSources();
		if (Sources.Num() == 0)
		{
			TSharedPtr<FSourceListItem> Item = MakeShared<FSourceListItem>();
			Item->Type = TEXT("（暂无源）");
			SourceListItems.Add(MoveTemp(Item));
		}
		else
		{
			for (const FGuid& SourceGuid : Sources)
			{
				TSharedPtr<FSourceListItem> Item = MakeShared<FSourceListItem>();
				Item->Type = LiveLinkClient->GetSourceType(SourceGuid).ToString();
				Item->MachineName = LiveLinkClient->GetSourceMachineName(SourceGuid).ToString();
				Item->Status = LiveLinkClient->GetSourceStatus(SourceGuid).ToString();
				SourceListItems.Add(MoveTemp(Item));
			}
		}
	}

	if (SourceListView.IsValid())
	{
		SourceListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> FDoodleLiveLinkWindow::OnGenerateSourceRow(TSharedPtr<FSourceListItem> InItem,
                                                                 const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(STableRow<TSharedPtr<FSourceListItem>>, OwnerTable)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->Type))
				.MinDesiredWidth(160.0f)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->MachineName))
				.MinDesiredWidth(160.0f)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->Status))
			]
		];
}

void FDoodleLiveLinkWindow::OnWindowClosed(const TSharedRef<SWindow>& InWindow)
{
	RequestEngineExit(TEXT("DoodleLiveLink 主窗口已关闭"));
}
