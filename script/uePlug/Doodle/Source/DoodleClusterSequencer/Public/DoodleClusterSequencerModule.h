// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ISequencerModule.h"
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
// #include "GeometryCacheTrackEditor.h"

/**
 * The public interface to this module
 */
class FDoodleClusterSequencerModule : public IModuleInterface
{
public:
  virtual void StartupModule() override;

  virtual void ShutdownModule() override;

  FDelegateHandle TrackEditorBindingHandle;
};
