// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoodleLiveLinkWebSocketServer.h"

#include "DoodleLiveLink.h"
#include "INetworkingWebSocket.h"
#include "IWebSocketNetworkingModule.h"
#include "IWebSocketServer.h"
#include "Modules/ModuleManager.h"

FDoodleLiveLinkWebSocketServer::FDoodleLiveLinkWebSocketServer() = default;

FDoodleLiveLinkWebSocketServer::~FDoodleLiveLinkWebSocketServer()
{
	Stop();
}

void FDoodleLiveLinkWebSocketServer::Start(uint16 InPort)
{
	Stop();

	Port = InPort;

	IWebSocketNetworkingModule& WebSocketModule = FModuleManager::LoadModuleChecked<IWebSocketNetworkingModule>("WebSocketNetworking");
	Server = WebSocketModule.CreateServer();
	if (!Server)
	{
		UE_LOG(LogDoodleLiveLink, Error, TEXT("创建 WebSocket 服务器失败"));
		return;
	}

	FWebSocketClientConnectedCallBack ConnectedCallback;
	ConnectedCallback.BindRaw(this, &FDoodleLiveLinkWebSocketServer::OnClientConnected);

	if (!Server->Init(Port, ConnectedCallback))
	{
		UE_LOG(LogDoodleLiveLink, Error, TEXT("WebSocket 服务器启动失败（端口 %d）"), Port);
		Server.Reset();
		return;
	}

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FDoodleLiveLinkWebSocketServer::Tick));

	UE_LOG(LogDoodleLiveLink, Log, TEXT("WebSocket 服务器已启动（端口 %d）"), Port);
}

void FDoodleLiveLinkWebSocketServer::Stop()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

	Clients.Reset();
	Server.Reset();
}

void FDoodleLiveLinkWebSocketServer::Restart(uint16 InPort)
{
	Start(InPort);
}

bool FDoodleLiveLinkWebSocketServer::Tick(float DeltaTime)
{
	if (Server)
	{
		Server->Tick();
	}
	return true;
}

void FDoodleLiveLinkWebSocketServer::OnClientConnected(INetworkingWebSocket* Socket)
{
	if (!Socket)
	{
		return;
	}

	Clients.Add(Socket);

	// 断开时从列表移除，避免向已关闭的连接发送数据。
	const FString Endpoint = Socket->RemoteEndPoint(true);
	Socket->SetSocketClosedCallBack(FWebSocketInfoCallBack::CreateLambda([this, Socket, Endpoint]()
	{
		const int32 RemovedCount = Clients.Remove(Socket);
		if (RemovedCount > 0)
		{
			UE_LOG(LogDoodleLiveLink, Log, TEXT("WebSocket 客户端已断开：%s"), *Endpoint);
		}
	}));

	UE_LOG(LogDoodleLiveLink, Log, TEXT("WebSocket 客户端已连接：%s"), *Endpoint);
}

void FDoodleLiveLinkWebSocketServer::Broadcast(const TArray<uint8>& InPayload)
{
	if (InPayload.Num() == 0)
	{
		return;
	}

	for (INetworkingWebSocket* Client : Clients)
	{
		if (Client)
		{
			Client->Send(InPayload.GetData(), InPayload.Num(), false);
		}
	}
}
