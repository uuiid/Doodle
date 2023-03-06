/*
 * @Author: your name
 * @Date: 2020-12-03 11:41:51
 * @LastEditTime: 2020-12-03 20:17:54
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath:
 * \Usersd:\Source\UnrealEngine\Engine\Plugins\Doodle\Source\Doodle\Private\DoodleCommands.cpp
 */
// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoodleCommands.h"

#define LOCTEXT_NAMESPACE "FdoodleModule"

void FDoodleCommands::RegisterCommands() {
  UI_COMMAND(OpenPluginWindow, "Doodle", "Bring up doodle window", EUserInterfaceActionType::Button, FInputGesture());
  UI_COMMAND(
      DoodleImportFbxWindow, "Doodle Import Fbx", "Open Import Fbx Windows", EUserInterfaceActionType::Button,
      FInputGesture()
  );
}

#undef LOCTEXT_NAMESPACE
