// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoodleLiveLinkRun.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformSplash.h"
#include "LaunchEngineLoop.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoodleLiveLink, Log, All);

int32 RunDoodleLiveLink(const TCHAR* CommandLine)
{
	FTaskTagScope Scope(ETaskTag::EGameThread);

	// Needs to be initialized early for mount points / plugin search paths.
	FCommandLine::Set(CommandLine);

	const FText AppName = NSLOCTEXT("DoodleLiveLink", "SplashTextName", "Doodle Live Link");
	FPlatformSplash::SetSplashText(SplashTextType::GameName, AppName);

#if !UE_BUILD_SHIPPING
	if (FParse::Param(CommandLine, TEXT("WaitForDebugger")))
	{
		while (!FPlatformMisc::IsDebuggerPresent())
		{
			FPlatformProcess::Sleep(0.1f);
		}
		UE_DEBUG_BREAK();
	}
#endif

	FPlatformMisc::SetUBTTargetName(TEXT("DoodleLiveLink"));

	// 独立项目：exe 位于 <Project>/Binaries/Win64/，向上三级即项目根目录。
	// 未通过命令行传入 -project 时，自动定位 DoodleLiveLink.uproject。
	if (FPaths::GetProjectFilePath().IsEmpty())
	{
		const FString ExePath(FPlatformProcess::ExecutablePath());
		const FString ProjectDir = FPaths::GetPath(FPaths::GetPath(FPaths::GetPath(ExePath)));
		const FString ProjectFile = FPaths::Combine(ProjectDir, TEXT("DoodleLiveLink.uproject"));
		if (FPaths::FileExists(ProjectFile))
		{
			FPaths::SetProjectFilePath(ProjectFile);
		}
	}

	const TCHAR* const ExtraArgs = TEXT("-xrtrackingonly");
	int32 Result = GEngineLoop.PreInit(*FString::Printf(TEXT("%s %s %s"),
		*FPaths::GetProjectFilePath(), CommandLine, ExtraArgs));

	// Ensure FEngineLoop::Exit is called for all return paths.
	ON_SCOPE_EXIT
	{
		GEngineLoop.Exit();
	};

	if (Result != 0)
	{
		UE_LOG(LogDoodleLiveLink, Error, TEXT("EngineLoop PreInit failed (%i)"), Result);
	}

	if (Result != 0 || IsEngineExitRequested())
	{
		return Result;
	}

	{
		Result = GEngineLoop.Init();

		// Hide the splash screen now that everything is ready to go
		FPlatformSplash::Hide();

		while (!IsEngineExitRequested())
		{
			GEngineLoop.Tick();
		}
	}

	return Result;
}
