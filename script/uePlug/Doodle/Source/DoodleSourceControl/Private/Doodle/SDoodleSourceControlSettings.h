// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "SlateFwd.h"
#include "ISourceControlOperation.h"
#include "ISourceControlProvider.h"

class SDoodleSourceControlSettings : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SDoodleSourceControlSettings) {}

  SLATE_END_ARGS()

 public:
  void Construct(const FArguments& InArgs);
  ~SDoodleSourceControlSettings();

 private:
  void SetPathToRepositoryRoot(const FString& InPath);

  void AddNewFileRoot();
  SVerticalBox::FSlot* AddFileRootWidget_Expose;
  /// 显示的同步路径
  FString PathToRepositoryRoot;

};