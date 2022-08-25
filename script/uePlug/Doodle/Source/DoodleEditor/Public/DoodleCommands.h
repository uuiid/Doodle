// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "doodleStyle.h"

class FdoodleCommands : public TCommands<FdoodleCommands> {
 public:
  FdoodleCommands()
      : TCommands<FdoodleCommands>(TEXT("doodle"), NSLOCTEXT("Contexts", "doodle", "doodle Plugin"), NAME_None, FdoodleStyle::GetStyleSetName()) {
  }

  // TCommands<> interface
  virtual void RegisterCommands() override;

 public:
  TSharedPtr<FUICommandInfo> OpenPluginWindow;
};
