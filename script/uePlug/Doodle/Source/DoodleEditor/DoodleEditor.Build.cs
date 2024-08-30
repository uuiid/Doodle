// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

//using System.IO;

//这个模块只是用来创建不同时间段加载用的, 所以只用依赖这一个
public class doodleEditor : ModuleRules
{
	public doodleEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		/// 启用rtti, 用来使用 std::dynamic_pointer_cast
		//bForceEnableRTTI = true;

		PublicIncludePaths.AddRange(
			new string[]
			{
				// ... add public include paths required here ...
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
				// ... add other private include paths required here ...
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"ContentBrowser",
				"EditorScriptingUtilities",
				"UnrealEd",

				"Engine", //几何缓存的骨骼依赖
				// ... add other public dependencies that you statically link with here ...
				//abc的包装需要
				"AlembicLib",
				"MeshUtilities",
				"MaterialUtilities",
				"PropertyEditor",
				"SlateCore",
				"Slate",
				"EditorStyle",
				"Eigen",
				"RenderCore",
				"RHI",
				"DeveloperSettings", // 显示设置模块
				"EditorSubsystem", //子系统依赖
				"LevelEditor", // 关卡子系统依赖

				// 资产自定义模块
				// "ContentBrowserAssetDataSource"
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"doodle",


				"ContentBrowser",
				"EditorScriptingUtilities",
				"GeometryCache", //复制材质依赖
				"AssetTools", //资产工具 导入依赖

				"DesktopPlatform", //桌面依赖  打开文件对话框的依赖
				"RenderCore",
				"AlembicLibrary", //abc批量导入需要
				"AIModule", //这个是ai控制使用的
				"PlacementMode", //这个是创建面板用的
				"AssetRegistry", //资源注册表模块
				"JsonUtilities", // 批量导入需要读取json
				"Json", // 批量导入需要读取json
				"SourceControl", //源代码管理,

				"LevelSequence", //创建定序器
				"LevelSequenceEditor", // 导入定序器关卡
				"MovieSceneTracks", // 定序器中相机的设置模块
				"Sequencer", //修改定序器
				"MovieScene", //更新定序器场景
				"CinematicCamera", // 相机模块
				"MovieSceneTools", // /更新定序器场景
				"DesktopWidgets", // 打开文件对话框小步件
				"FBX", // fbx读取需要
				"ImageCore", // 调整图像使用
				"SkeletalMeshUtilitiesCommon", // 简化骨骼
				"AlembicImporter", //导入abc

				// 捏脸专用
				"AdvancedPreviewScene", // 高级预览场景, 这个需要做捏脸的时候用
				"AnimGraph", // 捏脸K帧, 这个需要做捏脸的时候用
				"SequenceRecorder", // 镜头录制
				"KismetWidgets", // 时间控制
				"CurveEditor", //添加曲线编辑模块
				"Persona", //骨骼
				"TimeManagement", // 添加时间管理
				//"Foliage", // 导出场景使用
				"MeshDescription", //修改sk使用
				"MeshBuilder", //修改sk使用
				"DoodleAbc", // 我们自己的导入

				"SequencerScriptingEditor",
				"MovieRenderPipelineCore", // 远程渲染需要
				"MovieRenderPipelineEditor", //远程渲染需要
				"GeometryCacheTracks", //导入缓存需要

				//-----------
				"Sockets",
				"Networking",
				"SequencerWidgets", // 曲线编辑器
				//---------
				"MovieRenderPipelineRenderPasses",
                "EditorWidgets",
                "Niagara",
                "AVIWriter",
                "MovieSceneCapture",
                "MediaAssets",
                "MediaPlayerEditor",
				"Media",
                "NiagaraEditor",
                "ToolWidgets",
                "MovieSceneCaptureDialog",
				"MovieRenderPipelineSettings",// 自动导入设置模块
				"SkeletalMeshEditor", /// 获取骨骼网格体信息
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