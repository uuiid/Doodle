// Copyright Epic Games, Inc. All Rights Reserved.

#include "doodle.h"
#include "doodleStyle.h"
#include "doodleCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

#include "doodleCopyMaterial.h"

static const FName doodleTabName("doodle");

#define LOCTEXT_NAMESPACE "FdoodleModule"

void FdoodleModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FdoodleStyle::Initialize();
	FdoodleStyle::ReloadTextures();

	FdoodleCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FdoodleCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FdoodleModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FdoodleModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(doodleTabName, FOnSpawnTab::CreateRaw(this, &FdoodleModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FdoodleTabTitle", "doodle"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FdoodleModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FdoodleStyle::Shutdown();

	FdoodleCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(doodleTabName);
}

TSharedRef<SDockTab> FdoodleModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FdoodleModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("doodle.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				SNew(DoodleCopyMat)//这里创建我们自己的界面
			]
		];
}

void FdoodleModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(doodleTabName);
}

void FdoodleModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FdoodleCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FdoodleCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FdoodleModule, doodle)