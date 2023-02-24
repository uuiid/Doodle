// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "doodleStyle.h"

class FDoodleCommands : public TCommands<FDoodleCommands> {
 public:
  FDoodleCommands()
      : TCommands<FDoodleCommands>(TEXT("Doodle"), NSLOCTEXT("Contexts", "Doodle", "Doodle Plugin"), NAME_None, FdoodleStyle::GetStyleSetName()) {
  }

  // TCommands<> interface
  virtual void RegisterCommands() override;

 public:
  TSharedPtr<FUICommandInfo> OpenPluginWindow, DoodleImportFbxWindow, DoodleCreateCharacter;
};
