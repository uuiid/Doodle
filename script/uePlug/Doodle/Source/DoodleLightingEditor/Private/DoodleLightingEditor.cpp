#include "DoodleLightingEditor.h"

#include "DoodleLightingRoundRunner.h"

#include "Framework/Application/SlateApplication.h"

void FDoodleLightingEditorModule::StartupModule()
{
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
