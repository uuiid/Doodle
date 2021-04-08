#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

//#include "DoodleEditor.generated.h"

class FdoodleEditorModule : public IModuleInterface {
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};