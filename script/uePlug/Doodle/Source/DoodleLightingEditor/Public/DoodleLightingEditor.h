#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FDoodleLightingEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
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
