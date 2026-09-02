// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

template <typename ItemType> class SListView;
class ITableRow;
class STableViewBase;
class FDoodleLiveLinkForwarder;
class FDoodleLiveLinkWebSocketServer;

/** DoodleLiveLink 中心主窗口。 */
class FDoodleLiveLinkWindow : public TSharedFromThis<FDoodleLiveLinkWindow>
{
public:
	FDoodleLiveLinkWindow();
	~FDoodleLiveLinkWindow();

	/** 创建并显示主窗口。 */
	void CreateWindow();

private:
	/** 源列表条目。 */
	struct FSourceListItem
	{
		FString Type;
		FString MachineName;
		FString Status;
	};

	/** 创建 Live Link Face 源。 */
	void CreateLiveLinkFaceSource();

	/** 刷新源列表显示。 */
	void RefreshSourceList();

	/** 生成源列表的一行。 */
	TSharedRef<ITableRow> OnGenerateSourceRow(TSharedPtr<FSourceListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable) const;

	/** 主窗口关闭时触发，请求退出引擎。 */
	void OnWindowClosed(const TSharedRef<SWindow>& InWindow);

	TSharedPtr<class SWindow> RootWindow;

	/** 源列表数据与控件。 */
	TArray<TSharedPtr<FSourceListItem>> SourceListItems;
	TSharedPtr<SListView<TSharedPtr<FSourceListItem>>> SourceListView;

	/** WebSocket 服务器及其端口配置。 */
	uint16 WebSocketServerPort = 8890;
	TUniquePtr<FDoodleLiveLinkWebSocketServer> WebSocketServer;

	/** Live Link Face 转发器。 */
	TUniquePtr<FDoodleLiveLinkForwarder> Forwarder;

	/** Live Link Face 源连接配置。 */
	FString LiveLinkFaceAddress = TEXT("127.0.0.1");
	uint16 LiveLinkFacePort = 14785;
	FString LiveLinkFaceSubjectName = TEXT("DoodleFace");
};
