// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoodleLiveLinkWebSocketServer.h"

#include "DoodleLiveLink.h"
#include "INetworkingWebSocket.h"
#include "IWebSocketNetworkingModule.h"
#include "IWebSocketServer.h"
#include "Modules/ModuleManager.h"

namespace
{
	/** 按 MessagePack 规范写入 array 头（fixarray / array16 / array32）。 */
	void AppendArrayHeader(TArray<uint8>& Out, int32 Count)
	{
		if (Count <= 0x0F)
		{
			Out.Add(static_cast<uint8>(0x90 | Count));
		}
		else if (Count <= 0xFFFF)
		{
			Out.Add(0xDC);
			Out.Add(static_cast<uint8>(Count >> 8));
			Out.Add(static_cast<uint8>(Count));
		}
		else
		{
			Out.Add(0xDD);
			Out.Add(static_cast<uint8>(Count >> 24));
			Out.Add(static_cast<uint8>(Count >> 16));
			Out.Add(static_cast<uint8>(Count >> 8));
			Out.Add(static_cast<uint8>(Count));
		}
	}

	/** 按 MessagePack 规范写入 float32（0xCA + 大端序 IEEE754）。 */
	void AppendFloat32(TArray<uint8>& Out, float Value)
	{
		Out.Add(0xCA);
		uint32 Bits = 0;
		FMemory::Memcpy(&Bits, &Value, sizeof(float));
		Out.Add(static_cast<uint8>(Bits >> 24));
		Out.Add(static_cast<uint8>(Bits >> 16));
		Out.Add(static_cast<uint8>(Bits >> 8));
		Out.Add(static_cast<uint8>(Bits));
	}

	/** 按 MessagePack 规范写入 str（fixstr / str8 / str16 / str32）。 */
	void AppendString(TArray<uint8>& Out, const FString& String)
	{
		FTCHARToUTF8 Utf8(*String);
		const int32 ByteCount = Utf8.Length();
		const uint8* Bytes = reinterpret_cast<const uint8*>(Utf8.Get());

		if (ByteCount <= 31)
		{
			Out.Add(static_cast<uint8>(0xA0 | ByteCount));
		}
		else if (ByteCount <= 0xFF)
		{
			Out.Add(0xD9);
			Out.Add(static_cast<uint8>(ByteCount));
		}
		else if (ByteCount <= 0xFFFF)
		{
			Out.Add(0xDA);
			Out.Add(static_cast<uint8>(ByteCount >> 8));
			Out.Add(static_cast<uint8>(ByteCount));
		}
		else
		{
			Out.Add(0xDB);
			Out.Add(static_cast<uint8>(ByteCount >> 24));
			Out.Add(static_cast<uint8>(ByteCount >> 16));
			Out.Add(static_cast<uint8>(ByteCount >> 8));
			Out.Add(static_cast<uint8>(ByteCount));
		}

		Out.Append(Bytes, ByteCount);
	}

	/** 将属性值数组打包为 MessagePack array（元素为 float32）。 */
	TArray<uint8> PackPropertyValues(const TArray<float>& InValues)
	{
		TArray<uint8> Payload;
		AppendArrayHeader(Payload, InValues.Num());
		for (float Value : InValues)
		{
			AppendFloat32(Payload, Value);
		}
		return Payload;
	}

	/** 将属性名数组打包为 MessagePack array（元素为 str）。 */
	TArray<uint8> PackPropertyNames(const TArray<FName>& InNames)
	{
		TArray<uint8> Payload;
		AppendArrayHeader(Payload, InNames.Num());
		for (const FName& Name : InNames)
		{
			AppendString(Payload, Name.ToString());
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

	const TArray<uint8> Payload = PackPropertyNames(PropertyNamesProvider());
	if (Payload.Num() > 0)
	{
		Socket->Send(Payload.GetData(), Payload.Num(), false);
	}
}

void FDoodleLiveLinkWebSocketServer::Broadcast(const FLiveLinkBaseFrameData& InFrameData)
{
	// 将属性值打包为 MessagePack array 后广播。
	const TArray<uint8> Payload = PackPropertyValues(InFrameData.PropertyValues);
	if (Payload.Num() == 0)
	{
		return;
	}

	for (INetworkingWebSocket* Client : Clients)
	{
		if (Client)
		{
			Client->Send(Payload.GetData(), Payload.Num(), false);
		}
	}
}
