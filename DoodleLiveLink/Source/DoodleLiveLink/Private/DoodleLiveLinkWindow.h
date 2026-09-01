// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "ILiveLinkSource.h"

template <typename ItemType> class SListView;
class ITableRow;
class STableViewBase;
class IWebSocketServer;
class INetworkingWebSocket;
class ULiveLinkRole;

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

	/** 启动 / 停止 / 重启 WebSocket 服务器。 */
	void StartWebSocketServer();
	void StopWebSocketServer();
	void RestartWebSocketServer();

	/** 每帧驱动 WebSocket 服务器。 */
	bool TickWebSocketServer(float DeltaTime);

	/** WebSocket 客户端连接回调。 */
	void OnWebSocketClientConnected(INetworkingWebSocket* Socket);

	/** 注册 Live Link Face 帧数据回调。 */
	void RegisterLiveLinkFaceFrameCallback();

	/** 注销 Live Link Face 帧数据回调。 */
	void UnregisterLiveLinkFaceFrameCallback();

	/** 尝试为当前 Subject 注册帧数据委托。 */
	void TryRegisterLiveLinkFaceFrames();

	/** 主题添加时，若匹配当前 Subject 名则注册帧回调。 */
	void OnLiveLinkSubjectAdded(FLiveLinkSubjectKey InSubjectKey);

	/** Live Link Face 静态数据 / 帧数据回调。 */
	void OnLiveLinkFaceStaticData(FLiveLinkSubjectKey InSubjectKey, TSubclassOf<ULiveLinkRole> InSubjectRole, const FLiveLinkStaticDataStruct& InStaticData);
	void OnLiveLinkFaceFrameData(FLiveLinkSubjectKey InSubjectKey, TSubclassOf<ULiveLinkRole> InSubjectRole, const FLiveLinkFrameDataStruct& InFrameData);

	/** 将数据广播到所有 WebSocket 客户端。 */
	void BroadcastToWebSocketClients(const TArray<uint8>& InPayload);

	TSharedPtr<class SWindow> RootWindow;

	/** 源列表数据与控件。 */
	TArray<TSharedPtr<FSourceListItem>> SourceListItems;
	TSharedPtr<SListView<TSharedPtr<FSourceListItem>>> SourceListView;

	/** WebSocket 服务器及其端口配置。 */
	uint16 WebSocketServerPort = 8890;
	TUniquePtr<IWebSocketServer> WebSocketServer;
	FTSTicker::FDelegateHandle WebSocketTickerHandle;

	/** 已连接的 WebSocket 客户端。 */
	TArray<INetworkingWebSocket*> WebSocketClients;

	/** Live Link Face 帧数据回调句柄与属性名。 */
	FDelegateHandle LiveLinkSubjectAddedHandle;
	FDelegateHandle LiveLinkFaceStaticDataHandle;
	FDelegateHandle LiveLinkFaceFrameDataHandle;
	TArray<FName> LiveLinkFacePropertyNames;

	/** 已创建的 Live Link Face 源句柄。 */
	FLiveLinkSourceHandle LiveLinkFaceSourceHandle;
	bool bLiveLinkFaceSourceCreated = false;
	bool bLiveLinkFaceConnected = false;

	/** Live Link Face 源连接配置。 */
	FString LiveLinkFaceAddress = TEXT("127.0.0.1");
	uint16 LiveLinkFacePort = 14785;
	FString LiveLinkFaceSubjectName = TEXT("DoodleFace");
};
