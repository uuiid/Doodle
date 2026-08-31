// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoodleLiveLinkRun.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformSplash.h"
#include "LaunchEngineLoop.h"
#include "Modules/ModuleManager.h"

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

	// in-tree 开发构建：uproject 位于 Engine/Source/Programs/DoodleLiveLink/ 下。
	const TCHAR* const DevelopmentProjectPath = TEXT("../../Source/Programs/DoodleLiveLink/DoodleLiveLink.uproject");
	// Cooked Editor 分发构建：uproject 位于 <发布目录>/DoodleLiveLink/ 下。
	const TCHAR* const StagedProjectPath = TEXT("../../../DoodleLiveLink/DoodleLiveLink.uproject");

	if (FPaths::FileExists(DevelopmentProjectPath))
	{
		FPaths::SetProjectFilePath(DevelopmentProjectPath);
	}
	else if (FPaths::FileExists(StagedProjectPath))
	{
		FPaths::SetProjectFilePath(StagedProjectPath);

		// 确保分发版中由 hub 生成的内容（如 LiveLink 录制）可被找到。
		const FString GameContentPath = FPaths::Combine(FPlatformProcess::UserSettingsDir(), *FApp::GetEpicProductIdentifier(), TEXT("DoodleLiveLink"), TEXT("Content"));
		FPackageName::RegisterMountPoint(TEXT("/Game/"), GameContentPath);
	}

	FPlatformMisc::SetUBTTargetName(TEXT("DoodleLiveLink"));

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

		// 加载主模块，创建 DoodleLiveLink 中心窗口（此时 Slate 已就绪）
		FModuleManager::Get().LoadModuleChecked("DoodleLiveLink");

		while (!IsEngineExitRequested())
		{
			GEngineLoop.Tick();
		}
	}

	return Result;
}
