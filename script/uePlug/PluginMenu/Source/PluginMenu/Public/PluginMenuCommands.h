// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "PluginMenuStyle.h"

class FPluginMenuCommands : public TCommands<FPluginMenuCommands>
{
public:

	FPluginMenuCommands()
		: TCommands<FPluginMenuCommands>(TEXT("PluginMenu"), NSLOCTEXT("Contexts", "PluginMenu", "PluginMenu Plugin"), NAME_None, FPluginMenuStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
	TSharedPtr< FUICommandInfo > OpenMenu1_1UI, OpenMenu1_2UI, OpenMenu1_3UI, OpenMenu1_4UI, OpenMenu1_5UI, OpenMenu1_6UI, OpenMenu1_7UI;

};