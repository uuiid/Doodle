// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PluginMenuCommands.h"

#define LOCTEXT_NAMESPACE "FPluginMenuModule"

void FPluginMenuCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "PluginMenu", "Bring up PluginMenu window", EUserInterfaceActionType::Button, FInputGesture());


	UI_COMMAND(OpenMenu1_1UI, "MapGenerator", "Bring up PluginMenu window", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(OpenMenu1_2UI, "MapGenerator", "Bring up PluginMenu window", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(OpenMenu1_3UI, "MapGenerator", "Bring up PluginMenu window", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(OpenMenu1_4UI, "MapGenerator", "Bring up PluginMenu window", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(OpenMenu1_5UI, "MapGenerator", "Bring up PluginMenu window", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(OpenMenu1_6UI, "MapGenerator", "Bring up PluginMenu window", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(OpenMenu1_7UI, "MapGenerator", "Bring up PluginMenu window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
