// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
//using System.IO;

public class doodle : ModuleRules
{
    public doodle(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
            );



        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Engine",
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "RenderCore",
                "RHI",
                //几何缓存依赖
                "GeometryCache",
                // ai模块
                "AIModule",

            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "RenderCore",
                "RHI",

                "AIModule",//我们需要AI模块
                "MovieScene",//更新定序器场景
                "LevelSequence",//创建定序器
                // "Sequencer",//修改定序器(这个模块不可以被核心依赖)
                "MovieScene",//更新定序器场景
                "CinematicCamera",// 相机模块
                // "SequencerScriptingEditor", // 编辑器
                "MovieSceneTracks", // 骨骼物体轨道需要
                "NavigationSystem",// 导航网格
                "AnimGraphRuntime"// 动画评估模块
                // "Projects",
                // "InputCore",
                // "CoreUObject",
                // "Engine",
                // "Slate",
                // "SlateCore",
                // "ContentBrowserAssetDataSource"


				// ... add private dependencies that you statically link with here ...	
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );
    }
}
