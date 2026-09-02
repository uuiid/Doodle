// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "LiveLinkTypes.h"
#include "Templates/Function.h"

class IWebSocketServer;
class INetworkingWebSocket;

/** DoodleLiveLink WebSocket 服务器：自管理 Ticker 与客户端列表。 */
class FDoodleLiveLinkWebSocketServer
{
public:
	FDoodleLiveLinkWebSocketServer();
	~FDoodleLiveLinkWebSocketServer();

	void Start(uint16 InPort);
	void Stop();
	void Restart(uint16 InPort);

	/** 将帧数据序列化后广播到所有已连接客户端。 */
	void Broadcast(const FLiveLinkBaseFrameData& InFrameData);

	/** 设置属性名提供者：客户端发来消息时回传属性名列表。 */
	void SetPropertyNamesProvider(TFunction<TArray<FName>()> InProvider);

private:
	/** 每帧驱动服务器。 */
	bool Tick(float DeltaTime);

	/** 客户端连接回调。 */
	void OnClientConnected(INetworkingWebSocket* Socket);

	/** 客户端消息回调：回传属性名列表。 */
	void OnClientMessage(INetworkingWebSocket* Socket, void* Data, int32 DataSize);

	uint16 Port = 8890;
	TUniquePtr<IWebSocketServer> Server;
	FTSTicker::FDelegateHandle TickerHandle;
	TArray<INetworkingWebSocket*> Clients;

	TFunction<TArray<FName>()> PropertyNamesProvider;
};
