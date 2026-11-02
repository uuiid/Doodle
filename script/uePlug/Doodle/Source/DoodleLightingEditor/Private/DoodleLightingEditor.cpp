#include "DoodleLightingEditor.h"

#include "SDoodleKitsuLoginTab.h"

#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "DoodleLightingEditor"

void FDoodleLightingEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(SDoodleKitsuLoginTab::TabName, FOnSpawnTab::CreateStatic(&SDoodleKitsuLoginTab::OnSpawnAction))
		.SetDisplayName(LOCTEXT("DoodleKitsuLoginTabTitle", "Doodle 本机服务"))
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
		LOCTEXT("OpenDoodleKitsuLoginLabel", "AI 灯光会话"),
		LOCTEXT("OpenDoodleKitsuLoginToolTip", "打开 doodle 本机服务状态窗口"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]() { FGlobalTabmanager::Get()->TryInvokeTab(SDoodleKitsuLoginTab::TabName); }))
	);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDoodleLightingEditorModule, DoodleLightingEditor)
