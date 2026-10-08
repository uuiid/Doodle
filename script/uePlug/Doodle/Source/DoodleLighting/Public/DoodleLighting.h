#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

/**
 * AI 灯光数据层模块（Runtime）。
 *
 * 只承载 IR / wire 契约与报告读写，不做任何世界操作 —— 世界操作在 DoodleLightingEditor 里。
 */
class FDoodleLightingModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static inline FDoodleLightingModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FDoodleLightingModule>("DoodleLighting");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("DoodleLighting");
	}
};
