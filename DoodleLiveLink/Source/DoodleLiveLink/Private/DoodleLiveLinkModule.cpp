// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoodleLiveLink.h"
#include "DoodleLiveLinkWindow.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDoodleLiveLink);

class FDoodleLiveLinkModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogDoodleLiveLink, Log, TEXT("DoodleLiveLink 模块已启动"));
		// launcher 在 GEngineLoop::Init() 之后加载本模块，此时 Slate 已就绪，可直接创建窗口。
		Window = MakeShared<FDoodleLiveLinkWindow>();
		Window->CreateWindow();
		// TODO: 启动 WebSocket 服务端（UE 为服务端，Maya 为客户端）
	}

	virtual void ShutdownModule() override
	{
		Window.Reset();
		UE_LOG(LogDoodleLiveLink, Log, TEXT("DoodleLiveLink 模块已关闭"));
	}

private:
	TSharedPtr<class FDoodleLiveLinkWindow> Window;
};

IMPLEMENT_MODULE(FDoodleLiveLinkModule, DoodleLiveLink);
