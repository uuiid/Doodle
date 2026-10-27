#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

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
