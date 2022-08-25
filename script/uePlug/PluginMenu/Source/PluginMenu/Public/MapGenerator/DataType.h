#pragma once
#include "CoreMinimal.h"

struct FBPInfo {
  FString BPPackage;
  int32 StartFrame;
  bool bLoaded;

  FBPInfo(FString InBPPackage) {
    BPPackage  = InBPPackage;
    StartFrame = 71;
    bLoaded    = true;
  };
};

struct FMapInfo {
  FString MapPackage;
  bool bLoaded;

  FMapInfo(FString InMapPackage) {
    MapPackage = InMapPackage;
    bLoaded    = true;
  };
};

struct FFolderInfo {
  FString FolderPackage;
  bool bFolder;
  bool bAssetType;

  FFolderInfo(FString InFolderPackage) {
    FolderPackage = InFolderPackage;
    bFolder       = true;
    bAssetType    = false;
  };

  FFolderInfo(FString InFolderPackage, bool bMoveFolder, bool bMoveAsset) {
    FolderPackage = InFolderPackage;
    bFolder       = bMoveFolder;
    bAssetType    = bMoveAsset;
  };
};

struct FMenuEntryInfo {
  TSharedPtr<FUICommandInfo> UICommand;
  FString Title;
  FString ToolTip;

  FMenuEntryInfo(TSharedPtr<FUICommandInfo> InUICommand) {
    UICommand = InUICommand;
    Title     = "";
    ToolTip   = "";
  };

  FMenuEntryInfo(TSharedPtr<FUICommandInfo> InUICommand, FString InTitle, FString InToolTip) {
    UICommand = InUICommand;
    Title     = InTitle;
    ToolTip   = InToolTip;
  };
};
