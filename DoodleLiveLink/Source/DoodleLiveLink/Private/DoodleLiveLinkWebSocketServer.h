// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/Ticker.h"
#include "CoreMinimal.h"

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

	/** 广播数据到所有已连接客户端。 */
	void Broadcast(const TArray<uint8>& InPayload);

private:
	/** 每帧驱动服务器。 */
	bool Tick(float DeltaTime);

	/** 客户端连接回调。 */
	void OnClientConnected(INetworkingWebSocket* Socket);

	uint16 Port = 8890;
	TUniquePtr<IWebSocketServer> Server;
	FTSTicker::FDelegateHandle TickerHandle;
	TArray<INetworkingWebSocket*> Clients;
};
