#include "DoodleLightingEditor.h"

#include "DoodleLightingRoundRunner.h"

#include "Framework/Application/SlateApplication.h"

void FDoodleLightingEditorModule::StartupModule()
{
	// 命令行带 -DoodleAiLightParams= 时，这个进程就是「渲染一轮 AI 灯光候选」的进程。
	// 挂 OnPreTick 是为了等资产注册表扫完再动资产（照 DoodleEditor 模块里 UDoodleAutoRender 的写法）。
	if (!UDoodleLightingRoundRunner::HasAiLightParams())
	{
		return;
	}

	UDoodleLightingRoundRunner* L_Runner = NewObject<UDoodleLightingRoundRunner>(GetTransientPackage());
	L_Runner->AddToRoot();
	OnPreTickHandle = FSlateApplication::Get().OnPreTick().AddUObject(L_Runner, &UDoodleLightingRoundRunner::Main);
}

void FDoodleLightingEditorModule::ShutdownModule()
{
	if (OnPreTickHandle.IsValid())
	{
		FSlateApplication::Get().OnPreTick().Remove(OnPreTickHandle);
		OnPreTickHandle.Reset();
	}
}

IMPLEMENT_MODULE(FDoodleLightingEditorModule, DoodleLightingEditor)
