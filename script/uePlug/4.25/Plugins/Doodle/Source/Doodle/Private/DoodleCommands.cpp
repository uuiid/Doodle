// Copyright Epic Games, Inc. All Rights Reserved.

#include "doodleCommands.h"

#define LOCTEXT_NAMESPACE "FdoodleModule"

void FdoodleCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "doodle", "Bring up doodle window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
