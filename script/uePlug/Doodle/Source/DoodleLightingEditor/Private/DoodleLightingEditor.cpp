#include "DoodleLightingEditor.h"

#include "DoodleLightingRoundRunner.h"
#include "SDoodleKitsuLoginTab.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "DoodleLightingEditor"

void FDoodleLightingEditorModule::StartupModule()
{
	if (UDoodleLightingRoundRunner::HasAiLightParams())
	{
		UDoodleLightingRoundRunner* L_Runner = NewObject<UDoodleLightingRoundRunner>(GetTransientPackage());
		L_Runner->AddToRoot();
		OnPreTickHandle = FSlateApplication::Get().OnPreTick().AddUObject(L_Runner, &UDoodleLightingRoundRunner::Main);
	}

	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(SDoodleKitsuLoginTab::TabName, FOnSpawnTab::CreateStatic(&SDoodleKitsuLoginTab::OnSpawnAction))
		.SetDisplayName(LOCTEXT("DoodleKitsuLoginTabTitle", "Doodle Kitsu 登录"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDoodleLightingEditorModule::RegisterMenus));
}

void FDoodleLightingEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SDoodleKitsuLoginTab::TabName);
	}

	if (OnPreTickHandle.IsValid())
	{
		FSlateApplication::Get().OnPreTick().Remove(OnPreTickHandle);
		OnPreTickHandle.Reset();
	}
}

void FDoodleLightingEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped L_OwnerScoped(this);

	UToolMenu* L_Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	if (L_Menu == nullptr)
	{
		return;
	}

	FToolMenuSection& L_Section = L_Menu->FindOrAddSection("DoodleLighting", LOCTEXT("DoodleLightingSection", "Doodle Lighting"));
	L_Section.AddMenuEntry(
		"OpenDoodleKitsuLogin",
		LOCTEXT("OpenDoodleKitsuLoginLabel", "Kitsu 登录"),
		LOCTEXT("OpenDoodleKitsuLoginToolTip", "打开 doodle kitsu 账号登录窗口"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]() { FGlobalTabmanager::Get()->TryInvokeTab(SDoodleKitsuLoginTab::TabName); }))
	);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDoodleLightingEditorModule, DoodleLightingEditor)
