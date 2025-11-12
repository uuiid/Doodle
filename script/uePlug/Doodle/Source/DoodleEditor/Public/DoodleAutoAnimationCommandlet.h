// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "LevelSequence.h"
#include "Engine/DirectionalLight.h"

#include "DoodleAutoAnimationCommandlet.generated.h"

class ASkeletalMeshActor;

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
	UPROPERTY()
	TObjectPtr<USkeleton> Skeleton;
	TArray<FString> HideMaterials;
};

USTRUCT()
struct FImageSize
{
	GENERATED_BODY()

	int32 Width = 0, Height = 0;
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
	void ImportRig(const FString& InCondigPath);

private:
	/// 删除资产
	static void DeleteAsseet(const FString& InPath);


	void AddSequenceWorldToRenderWorld();


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
	/// 修改后处理框
	void PostProcessVolumeConfig();

	/// 导入fbx中的相机
	void ImportCamera(const FString& InFbxPath) const;
	/// 创建几何缓存导入任务
	UAssetImportTask* CreateGeometryImportTask(const FString& InFbxPath) const;
	/// 创建角色导入任务
	UAssetImportTask* CreateCharacterImportTask(const FString& InFbxPath, const TObjectPtr<USkeleton>& InSkeleton,
	                                            bool bImportAnimations = true) const;

	/// 隐藏材质
	void HideMaterials(const ASkeletalMeshActor* InActor) const;

	// 修复材质属性
	static void FixMaterialProperty();
	/// 修复5.4版本的材质参数集问题
	static void FixMaterialParameterCollection();
	/// 主要的渲染队列
	UPROPERTY()
	TObjectPtr<ULevelSequence> TheLevelSequence;
	FString DestinationPath;
	FString SequencePath;
	FString ImportPath;

	FFrameRate Rate{25, 1};
	FFrameRate TickRate{60000, 1};
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
	UPROPERTY()
	TObjectPtr<UWorld> TheSequenceWorld;

	/// 渲染关卡
	UPROPERTY()
	TObjectPtr<UWorld> TheRenderWorld;

	FFrameNumber L_Start{1001};
	FFrameNumber L_End{1200};
	int32 FrameTick{};
	//-----------
	TArray<FImportFiles2> ImportFiles;

	// 主光源
	UPROPERTY()
	TObjectPtr<ADirectionalLight> MainDirectionalLight;

	// 检查文件类型
	ECheckFileType CheckFileType;

	// 输出大小
	FImageSize ImageSize;
	bool Layering;
};

class UMoviePipelineExecutorBase;

UCLASS()
class UDoodleAutoRender : public UObject
{
	GENERATED_BODY()
	void OnEnd(UMoviePipelineExecutorBase*, bool);

public:
	void Main(float InTime);
	// 命令行中有我们指示渲染的参数
	static bool HasRenderParam();

	FDelegateHandle OnPreTickDelegateHandle;
};
