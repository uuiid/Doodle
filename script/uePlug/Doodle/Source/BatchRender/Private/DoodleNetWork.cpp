


#include "DoodleNetWork.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"

DoodleNetWork::DoodleNetWork()
{
    TMap<FString, FString> Header = { {"user","doodle"} };
    Socket = FWebSocketsModule::Get().CreateWebSocket(ServerURL, ServerProtocol, Header);
    Socket->OnConnected().AddRaw(this, &DoodleNetWork::OnConnected);
    Socket->OnConnectionError().AddRaw(this, &DoodleNetWork::OnConnectionError);
    /*Socket->OnClosed().AddRaw(this, &DoodleNetWork::OnConnectionClosed);
    Socket->OnMessage().AddRaw(this, &DoodleNetWork::OnMessageReceived);
    Socket->OnMessageSent().AddRaw(this, &DoodleNetWork::OnMessageSent);*/
    Connect();
}

DoodleNetWork::~DoodleNetWork()
{
    if (Socket) {
        Socket->Close();
    }
}

void DoodleNetWork::OnConnected()
{
    UE_LOG(LogTemp, Display, TEXT("OnConnected"));
}

void DoodleNetWork::OnConnectionError(const FString& Error)
{
    UE_LOG(LogTemp, Display, TEXT("OnConnectionError:%s"),*Error);
}

void DoodleNetWork::Connect()
{
    Socket->Connect();
}

void DoodleNetWork::Send(const FString& Message)
{
    if (Socket) {
        Socket->Send(Message);
    }
}
