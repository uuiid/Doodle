// AI 灯光执行器的数据层（Runtime，不依赖编辑器）。
//
// 模块分工（方案 §7.6）：
//   DoodleLighting      —— IR / Spec 数据结构、JSON 读写、op 语义、报告读写。可脱离渲染单独使用，
//                          也便于将来抽出来做纯 C++ 单测。
//   DoodleLightingEditor—— 只放 commandlet 与执行器（世界操作 + MovieRenderPipeline 渲染），无面板。
//
// 这里**故意**不依赖 UnrealEd / MovieRenderPipeline：渲染以外的部分不该把编辑器依赖带进来。

using UnrealBuildTool;

public class DoodleLighting : ModuleRules
{
	public DoodleLighting(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Json",
				"JsonUtilities",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// 数据层不需要编辑器能力，保持为空是有意的。
			}
		);
	}
}
