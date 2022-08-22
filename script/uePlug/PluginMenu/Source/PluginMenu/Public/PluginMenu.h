// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class SDockTab;
class FSpawnTabArgs;
struct FMenuEntryInfo;


class FPluginMenuModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void MenuEntryClickedHandle(FName TabName);

	void AddPullDownMenu(FMenuBarBuilder& MenuBarBuilder);
	void FillMenu(FMenuBuilder& MenuBarBuilder);
	void FillSubMenu1(FMenuBuilder& MenuBarBuilder);
	void AddSubMenuEntry(TArray<FMenuEntryInfo> SubMenuEntries);

	TSharedPtr<FExtender> MenuExtender;

	TArray<FMenuEntryInfo> Menu1SubMenuEntries;

	TArray<FName> TabNames;
	FName FindPlaceholderTab();
	void OpenMenuTab(FName TabName);
	
private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedRef<SDockTab> OnSpawnMenu1_1UITab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSpawnMenu1_2UITab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSpawnMenu1_3UITab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSpawnMenu1_4UITab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSpawnMenu1_5UITab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSpawnMenu1_6UITab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSpawnMenu1_7UITab(const FSpawnTabArgs& SpawnTabArgs);

	template <typename T>
	TSharedRef<SDockTab> OnSpawnMenuTabHandle(const FSpawnTabArgs& SpawnTabArgs, TSharedRef<T> MenuSlateUI)
	{
		return SNew(SDockTab)
			[
				MenuSlateUI
			];
	};


private:
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<class FTabManager> MenuTabManager;
};
