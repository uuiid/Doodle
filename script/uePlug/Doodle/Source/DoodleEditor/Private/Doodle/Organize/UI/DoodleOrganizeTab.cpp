// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/DoodleOrganizeTab.h"

#include "Doodle/Organize/UI/SDoodleOrganizePanel.h"

const FName SDoodleOrganizeTab::Name{ TEXT("DoodleOrganizeCompoundWidget") };

void SDoodleOrganizeTab::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SAssignNew(Panel, SDoodleOrganizePanel)
	];
}

TSharedRef<SDockTab> SDoodleOrganizeTab::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SDoodleOrganizeTab)
		];
}
