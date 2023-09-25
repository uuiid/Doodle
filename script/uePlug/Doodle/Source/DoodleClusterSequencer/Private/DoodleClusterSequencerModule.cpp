// Copyright Epic Games, Inc. All Rights Reserved.
#include "DoodleClusterSequencerModule.h"

#include "Doodle/DoodleLightningPostSequencerEditor.h"
#include "DoodleClusterSequencerEditor.h"
#include "ISequencerModule.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#define LOCTEXT_NAMESPACE "DoodleClusterSequencer"

IMPLEMENT_MODULE(FDoodleClusterSequencerModule, DoodleClusterSequencer);

void FDoodleClusterSequencerModule::StartupModule() {
  ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
  TrackEditorBindingHandle          = SequencerModule.RegisterTrackEditor(
      FOnCreateTrackEditor::CreateStatic(&FDoodleClusterTrackEditor::CreateTrackEditor)
  );
  TrackEditorBindingHandle2 = SequencerModule.RegisterTrackEditor(
      FOnCreateTrackEditor::CreateStatic(&FDoodleLightningPostSequencerEditor::CreateTrackEditor)
  );
}

void FDoodleClusterSequencerModule::ShutdownModule() {
  ISequencerModule* SequencerModulePtr = FModuleManager::Get().GetModulePtr<ISequencerModule>("Sequencer");
  if (SequencerModulePtr) {
    SequencerModulePtr->UnRegisterTrackEditor(TrackEditorBindingHandle);
    SequencerModulePtr->UnRegisterTrackEditor(TrackEditorBindingHandle2);
  }
}

#undef LOCTEXT_NAMESPACE
