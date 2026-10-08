#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * AI 灯光执行器模块（Editor）。
 *
 * 只做一件事：一轮渲染的驱动者 `UDoodleLightingRoundRunner` —— 命令行带 `-DoodleAiLightParams=` 时，
 * 本模块在启动时把它挂到 `OnPreTick` 上（MRQ 只在 PIE 世界里渲染，理由见那个类的说明）。
 *
 * 早期手工调试用的 `UDoodleLightingCommandlet`（`-run=DoodleLighting`，换编辑器世界 + SceneCapture 抓静图）
 * 已经删除：候选产物是视频，那条路既抓不出视频、也不再有调用者。
 *
 * 真正的 IR 数据结构 / JSON / 换算都在 Runtime 模块 `DoodleLighting` 里。
 */
class FDoodleLightingEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/// `UDoodleLightingRoundRunner::Main` 的注册句柄（正常路径是跑完就退进程，这里只是不让句柄悬着）
	FDelegateHandle OnPreTickHandle;

	static inline FDoodleLightingEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FDoodleLightingEditorModule>("DoodleLightingEditor");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("DoodleLightingEditor");
	}
};
