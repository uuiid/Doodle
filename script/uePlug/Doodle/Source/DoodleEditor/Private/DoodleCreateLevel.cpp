#include "DoodleCreateLevel.h"

//// 创建world
#include "AssetToolsModule.h"
#include "EditorLevelLibrary.h"
#include "Factories/WorldFactory.h"
#include "IAssetTools.h"
#include "LevelSequence.h"
#include "Modules/ModuleManager.h"
/// 定序器使用
#include "Doodle/DoodleImportFbxUI.h"
#include "Misc/OutputDeviceNull.h"
#include "MovieSceneToolHelpers.h"
#include "SequencerSettings.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
//// 保存操作使用
#include "FileHelpers.h"
/// 相机导入
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "MovieSceneObjectBindingID.h"


/// 资产注册表
#include "AssetRegistry/AssetRegistryModule.h"
/// 编译蓝图
#include "Kismet2/KismetEditorUtilities.h"
/// 编辑器脚本
#include "EditorAssetLibrary.h"

/// 自动导入类需要
#include "AssetImportTask.h"

/// 检查包名称存在
#include "Misc/PackageName.h"

/// 生成骨架网格体
#include "Animation/SkeletalMeshActor.h"
/// 动画
#include "Animation/AnimationAsset.h"

/// json需要
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
/// 几何缓存体使用
#include "GeometryCache.h"
/// 导入的fbx动画
#include "Animation/AnimSequence.h"
/// 几何缓存actor 导入
#include "GeometryCacheActor.h"
#include "GeometryCacheComponent.h"

/// 电影 Section
#include "MovieSceneSection.h"
/// 我们自定义的工具
#include "DoodleImportUiltEditor.h"
/// 导入相机的设置
#include "MovieSceneToolsUserSettings.h"

// 目录选择器
#include "Widgets/Input/SDirectoryPicker.h"
// 文件选择器
#include "AssetRegistry/IAssetRegistry.h"
#include "Widgets/Input/SFilePathPicker.h"
// 我们自己的多路径文件选择器
#include "Doodle/FilePathsPicker.h"
// 组合框
#include "Components/ComboBoxString.h"
// fbx读取需要
#include "FbxImporter.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
#include "fbxsdk/scene/animation/fbxanimcurve.h"
#include "fbxsdk/scene/animation/fbxanimlayer.h"
#include "fbxsdk/scene/animation/fbxanimstack.h"
#include "fbxsdk/scene/geometry/fbxcamera.h"
#include "fbxsdk/scene/geometry/fbxcameraswitcher.h"
#include "fbxsdk/scene/geometry/fbxnode.h"

// 读写文件
#include "Misc/FileHelper.h"
// 元数据
#include "UObject/MetaData.h"
// 算法
#include "Algo/AllOf.h"
/// 自动导入类需要
#include "AssetImportTask.h"

/// 正则
#include "Internationalization/Regex.h"
/// 一般的导入任务设置
#include "AssetImportTask.h"
/// 导入模块
#include "AssetToolsModule.h"
/// 导入fbx需要
#include "Animation/AnimBoneCompressionSettings.h"  // 压缩骨骼设置
#include "Factories/Factory.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxTextureImportData.h"
#include "Factories/ImportSettings.h"
#include "Factories/MaterialImportHelpers.h"
/// 进度框
#include "Misc/ScopedSlowTask.h"
/// 属性按钮
#include "PropertyCustomizationHelpers.h"
/// 内容游览器模块
#include "ContentBrowserModule.h"
/// 内容游览器
#include "IContentBrowserSingleton.h"
/// 导入abc
#include "AbcImportSettings.h"
/// 编辑器笔刷效果
#include "EditorStyleSet.h"

/// 导入相机需要的头文件
#include "Camera/CameraComponent.h"  // 相机组件
#include "CineCameraActor.h"         // 相机
#include "ILevelSequenceEditorToolkit.h"
#include "LevelSequence.h"
#include "MovieSceneToolHelpers.h"
#include "MovieSceneToolsUserSettings.h"          // 导入相机设置
#include "Sections/MovieSceneCameraCutSection.h"  // 相机剪切
#include "SequencerUtilities.h"                   // 创建相机

// 创建world
#include "AssetToolsModule.h"
#include "EditorLevelLibrary.h"
#include "Factories/WorldFactory.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "LevelEditorSubsystem.h"
#include "LevelSequence.h"
#include "Modules/ModuleManager.h"

// 导入abc
#include "AlembicImportFactory.h"
#include "Framework/Notifications/NotificationManager.h"  //通知管理类
#include "LevelEditorViewport.h"                          //编辑器视口
#include "Tracks/MovieSceneCameraCutTrack.h"              //处理对电影场景中CameraCut属性的操作。
#include "TransformData.h"                            //存储关于转换的信息，以便向转换部分添加键。
#include "Widgets/Notifications/SNotificationList.h"  // 编辑器通知

// 自定义导入abc
#include "Doodle/Abc/DoodleAbcImportSettings.h"
#include "Doodle/Abc/DoodleAlembicImportFactory.h"

/// 关卡编辑器子系统
#include "LevelEditorSubsystem.h"
#include "Misc/ScopeExit.h"  // 作用域退出

void FDoodleCreateLevel::ImportSkeletalMesh(const FString& InFbxpath)
{
}

void FDoodleCreateLevel::ImportGeometryCache(const FString& InAbcPath)
{
}

void FDoodleCreateLevel::ImportCamera(const FString& InFbxpath)
{
}

namespace
{
	bool IsCamera(UnFbx::FFbxImporter* InFbx)
	{
		TArray<fbxsdk::FbxCamera*> L_Cameras{};
		MovieSceneToolHelpers::GetCameras(InFbx->Scene->GetRootNode(), L_Cameras);
		return !L_Cameras.IsEmpty();
	}
} // namespace

void FDoodleCreateLevel::ImportFile(const FString& InFile)
{
	if (!FPaths::FileExists(InFile))
	{
		return;
	}

	FString l_ext = FPaths::GetExtension(InFile, true);

	if (l_ext == TEXT(".fbx"))
	{
		UnFbx::FFbxImporter* L_FbxImporter = UnFbx::FFbxImporter::GetInstance();
		L_FbxImporter->ImportFromFile(*InFile, FPaths::GetExtension(InFile));
		ON_SCOPE_EXIT { L_FbxImporter->ReleaseScene(); };
		if (IsCamera(L_FbxImporter))
		{
			ImportCamera(InFile);
		}
		else
		{
			L_FbxImporter->ReleaseScene();
			ImportSkeletalMesh(InFile);
		}
	}
	else if (l_ext == TEXT(".abc"))
	{
		ImportGeometryCache(InFile);
	}
}

void FDoodleCreateLevel::PreparationImport(const FString& InFile)
{
	if (!FPaths::FileExists(InFile))
	{
		return;
	}

	FString l_ext = FPaths::GetExtension(InFile, true);

	if (l_ext == TEXT(".fbx"))
	{
		UnFbx::FFbxImporter* L_FbxImporter = UnFbx::FFbxImporter::GetInstance();
		L_FbxImporter->ImportFromFile(*InFile, FPaths::GetExtension(InFile));
		ON_SCOPE_EXIT { L_FbxImporter->ReleaseScene(); };
		if (IsCamera(L_FbxImporter))
		{
			ImportCamera(InFile);
		}
		else
		{
			L_FbxImporter->ReleaseScene();
			ImportSkeletalMesh(InFile);
		}
	}
	else if (l_ext == TEXT(".abc"))
	{
		ImportGeometryCache(InFile);
	}
}

void FDoodleCreateLevel::ImportFiles(const TArray<FString>& InFiles)
{
	AllSkinObjs = FDoodleUSkeletonData_1::ListAllSkeletons();
	for (auto&& i : InFiles)
	{
		PreparationImport(i);
	}
	for (auto&& i : AllImportData)
	{
		i->ImportFile();
		UEditorAssetLibrary::SaveDirectory("/Game/Shot/");
	}
}

void FDoodleCreateLevel::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (auto&& i : AllImportData)
	{
		Collector.AddReferencedObject(i);
	}
}

FString FDoodleCreateLevel::GetReferencerName() const { return TEXT("Doodle Import"); }
