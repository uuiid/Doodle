// Copyright Epic Games, Inc. All Rights Reserved.

#include "HAL/Platform.h"
#include "RequiredProgramMainCPPInclude.h"

#if IS_PROGRAM
IMPLEMENT_APPLICATION(DoodleLiveLinkLauncher, "DoodleLiveLink");
#else
FEngineLoop GEngineLoop;
#endif
