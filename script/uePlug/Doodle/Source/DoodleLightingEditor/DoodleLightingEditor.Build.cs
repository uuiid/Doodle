// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

/// AI 灯光执行器（Editor）：只放「把 IR 落到 UE 世界」的那一层 + 一轮渲染的驱动者，不做面板。
/// 与 `doodleEditor` 分开是刻意的 —— 那边依赖一大堆导入/Sequencer 插件，这里只要渲染链路。
public class DoodleLightingEditor : ModuleRules
{
	public DoodleLightingEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				// IR 镜像 + JSON + 纯换算
				"DoodleLighting",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Json",
				"JsonUtilities",
				// 编辑器世界 / 资产注册表（不复制资产、不落盘，只做加载与必要的资产创建）
				"UnrealEd",
				"AssetRegistry",
				"AssetTools",
				"EditorSubsystem",
				"EditorScriptingUtilities",
				// 交付机位：ACineCameraActor 绑进 level sequence，再由 FBX 导入相机动画
				"LevelSequence",
				"MovieScene",
				"MovieSceneTracks",
				"CinematicCamera",
				"Sequencer",
				"MovieSceneTools",
				// 一轮渲染交给电影渲染管线（MRQ）：在 PIE 世界里逐候选出 PNG 帧序列，工作机再合成视频
				"MovieRenderPipelineCore",
				"MovieRenderPipelineEditor",
				"MovieRenderPipelineRenderPasses",
				"MovieRenderPipelineSettings",
				// `MoviePipelineEXROutput.h`（上面那个 RenderPasses 模块的公开头）会 include
				// `Imath/ImathBox.h` 与 OpenEXR 的头，而 RenderPasses 把 Imath/UEOpenExr 声明成
				// **Private** 依赖 —— 私有依赖不会传递给消费者，所以我们自己得再买一次单，
				// 否则编译到 `MoviePipelineEXROutput.h` 就 C1083 'Imath/ImathBox.h'。
				"Imath",
				"UEOpenExr",
				"UEOpenExrRTTI",
				"FBX",
				// 引擎路径 / 插件目录
				"Projects",
				// 编辑器面板（Part B）：Slate 控件 + 菜单/tab 注册
				"Slate",
				"SlateCore",
				"ToolMenus",
				"WorkspaceMenuStructure",
			}
		);
	}
}
