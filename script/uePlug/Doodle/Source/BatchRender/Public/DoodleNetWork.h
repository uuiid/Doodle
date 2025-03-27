

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
DECLARE_DELEGATE_TwoParams(FVfxImportDelegate, FString, FString)
DECLARE_DELEGATE_TwoParams(FVfxExportDelegate, bool, FString)
class BATCHRENDER_API DoodleNetWork : public TSharedFromThis<DoodleNetWork>
{
public:
	DoodleNetWork();
	~DoodleNetWork();
    void OnConnected();
    void OnConnectionError(const FString& Error);
    //void OnConnectionClosed(int32 StatusCode, const FString& Reason, bool bWasClean);

    //void OnMessageReceived(const FString& Message);
    //void OnMessageSent(const FString& MessageString);

    void Connect();
    //bool IsConnected();
    void Send(const FString& Message);
public:
    FVfxImportDelegate ImportEvent;
    FVfxExportDelegate ExportEvent;
private:
    const FString ServerURL = "ws://localhost:50024/socket.io/";
    const FString ServerProtocol = "ws";

    TSharedPtr<class IWebSocket> Socket = nullptr;
};
