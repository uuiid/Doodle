// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "LevelSequence.h"
#include "Engine/DirectionalLight.h"

#include "DoodleAutoAnimationCommandlet.generated.h"


enum class EImportFilesType2
{
	Camera,
	Geometry,
	Character,
};

enum class ECheckFileType
{
	Character,
	Scene,
};

USTRUCT()
struct FImportFiles2
{
	GENERATED_BODY()

	EImportFilesType2 Type;
	FString Path;
};


/**
 * 
 */
UCLASS()
class DOODLEEDITOR_API UDoodleAutoAnimationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UDoodleAutoAnimationCommandlet();

	virtual int32 Main(const FString& Params) override;


	void RunAutoLight(const FString& InCondigPath);
	void RunCheckFiles(const FString& InCondigPath);

private:
	/// 创建特效关卡
	void OnCreateEffectSequenceWorld();
	/// 创建特效定序器
	void OnCreateEffectSequence();
	/// 创建主关卡
	void OnCreateSequenceWorld();
	/// 创建主定序器
	void OnCreateSequence();

	void OnCreateDirectionalLight();
	void OnCreateCheckLight();
	void ClearAllLight();

	/// 主要灯光定序器中的导入数据
	void OnBuildSequence();
	/// 创建检查角色的定序器
	void OnBuildCheckCharacter();
	/// 创建检查场景的定序器
	void OnBuildCheckScene();


	/// 创建渲染配置
	void OnSaveReanderConfig();

	/// 导入fbx中的相机
	void ImportCamera(const FString& InFbxPath) const;
	/// 创建几何缓存导入任务
	UAssetImportTask* CreateGeometryImportTask(const FString& InFbxPath) const;
	/// 创建角色导入任务
	UAssetImportTask* CreateCharacterImportTask(const FString& InFbxPath) const;

	// 修复材质属性
	static void FixMaterialProperty();

	/// 主要的渲染队列
	ULevelSequence* TheLevelSequence;
	FString DestinationPath;
	FString SequencePath;
	FString ImportPath;
	FString EffectMapPath;

	FFrameRate Rate{25, 1};
	FFrameNumber Offset{50};

	/// 传入的主关卡, 用来拿到主关卡中的子关卡, 不进行渲染
	FString OriginalMapPath;
	/// 渲染关卡
	FString RenderMapPath;
	/// 导入UE中的文件所在的关卡
	FString CreateMapPath;
	/// 渲染配置
	FString MoviePipelineConfigPath;
	/// 主要的渲染队列关卡
	UWorld* TheSequenceWorld;

	FFrameNumber L_Start{1001};
	FFrameNumber L_End{1200};
	//-----------
	FString EffectSequencePath;
	TArray<FImportFiles2> ImportFiles;

	// 辅助光源
	ADirectionalLight* DirectionalLight1;
	// 主光源
	ADirectionalLight* DirectionalLight2;

	// 检查文件类型
	ECheckFileType CheckFileType;
};
