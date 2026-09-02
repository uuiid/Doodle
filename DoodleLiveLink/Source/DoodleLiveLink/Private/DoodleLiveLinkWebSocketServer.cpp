// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoodleLiveLinkWebSocketServer.h"

#include "DoodleLiveLink.h"
#include "INetworkingWebSocket.h"
#include "IWebSocketNetworkingModule.h"
#include "IWebSocketServer.h"
#include "Modules/ModuleManager.h"

namespace
{
	/** 将属性名列表序列化为字节流：int32 数量，随后每个名字 [int32 字节长度 + UTF-8 字节]。 */
	TArray<uint8> SerializePropertyNames(const TArray<FName>& InNames)
	{
		TArray<uint8> Payload;
		const int32 NameCount = InNames.Num();
		Payload.Append(reinterpret_cast<const uint8*>(&NameCount), sizeof(NameCount));
		for (const FName& Name : InNames)
		{
			FTCHARToUTF8 Utf8(*Name.ToString());
			const int32 ByteCount = Utf8.Length();
			Payload.Append(reinterpret_cast<const uint8*>(&ByteCount), sizeof(ByteCount));
			Payload.Append(reinterpret_cast<const uint8*>(Utf8.Get()), ByteCount);
		}
		return Payload;
	}
}

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

void FDoodleLiveLinkWebSocketServer::SetPropertyNamesProvider(TFunction<TArray<FName>()> InProvider)
{
	PropertyNamesProvider = MoveTemp(InProvider);
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

	// 收到客户端消息时回传属性名列表。
	Socket->SetReceiveCallBack(FWebSocketPacketReceivedCallBack::CreateLambda([this, Socket](void* Data, int32 DataSize)
	{
		OnClientMessage(Socket, Data, DataSize);
	}));

	UE_LOG(LogDoodleLiveLink, Log, TEXT("WebSocket 客户端已连接：%s"), *Endpoint);
}

void FDoodleLiveLinkWebSocketServer::OnClientMessage(INetworkingWebSocket* Socket, void* Data, int32 DataSize)
{
	if (!Socket || !PropertyNamesProvider)
	{
		return;
	}

	const TArray<uint8> Payload = SerializePropertyNames(PropertyNamesProvider());
	if (Payload.Num() > 0)
	{
		Socket->Send(Payload.GetData(), Payload.Num(), false);
	}
}

void FDoodleLiveLinkWebSocketServer::Broadcast(const FLiveLinkBaseFrameData& InFrameData)
{
	// 将属性值序列化为 float 数组字节流。
	TArray<uint8> Payload;
	const int32 ValueCount = InFrameData.PropertyValues.Num();
	const int32 ByteCount = ValueCount * sizeof(float);
	if (ByteCount == 0)
	{
		return;
	}

	Payload.SetNumUninitialized(ByteCount);
	FMemory::Memcpy(Payload.GetData(), InFrameData.PropertyValues.GetData(), ByteCount);

	for (INetworkingWebSocket* Client : Clients)
	{
		if (Client)
		{
			Client->Send(Payload.GetData(), Payload.Num(), false);
		}
	}
}
