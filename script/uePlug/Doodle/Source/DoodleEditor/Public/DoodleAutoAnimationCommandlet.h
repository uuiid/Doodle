// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "LevelSequence.h"
#include "Engine/DirectionalLight.h"

#include "DoodleAutoAnimationCommandlet.generated.h"


UENUM()
enum EImportFilesType2
{
	Camera,
	Geometry,
	Character,
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
private:
	void OnCreateEffectSequence();
	void OnCreateSequence();
	void OnCreateEffectSequenceWorld();
	void OnCreateSequenceWorld();
	void OnBuildSequence();
	void OnSaveReanderConfig();

	void ImportCamera(const FString& InFbxPath) const;

	/// 创建几何缓存导入任务
	UAssetImportTask* CreateGeometryImportTask(const FString& InFbxPath) const;
	/// 创建角色导入任务
	UAssetImportTask* CreateCharacterImportTask(const FString& InFbxPath) const;
	
	// 修复材质属性
	static void FixMaterialProperty();


	ULevelSequence* TheLevelSequence;
	FString DestinationPath;
	TArray<UAssetImportTask*> ImportTasks;
	TArray<UAssetImportTask*> ImportTasksAbc;
	FString SequencePath;
	FString ImportPath;

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
	FName EffectMapPath;
	ULevelSequence* EffectLevelSequence;
	UWorld* EffectSequenceWorld;
	TArray<FImportFiles2> ImportFiles;
	
	// 辅助光源
	ADirectionalLight* DirectionalLight1;
	// 主光源
	ADirectionalLight* DirectionalLight2;

};
