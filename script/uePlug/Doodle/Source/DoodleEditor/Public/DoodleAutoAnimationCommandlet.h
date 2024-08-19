// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "LevelSequence.h"
#include "Engine/DirectionalLight.h"

#include "DoodleAutoAnimationCommandlet.generated.h"

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
	
	// 修复材质属性
	void FixMaterialProperty();

	TSharedPtr<FJsonObject> JsonObject;
	ULevelSequence* TheLevelSequence;
	FString DestinationPath;
	TArray<UAssetImportTask*> ImportTasks;
	TArray<UAssetImportTask*> ImportTasksAbc;
	FString SequencePath;
	FString ImportPath;

	FName OriginalMapPath;
	FName RenderMapPath;
	FName CreateMapPath;

	FString MoviePipelineConfigPath;
	UWorld* TheSequenceWorld;

	FFrameNumber L_Start{1001};
	FFrameNumber L_End{1200};
	//-----------
	FString EffectSequencePath;
	FName EffectMapPath;
	ULevelSequence* EffectLevelSequence;
	UWorld* EffectSequenceWorld;
	
	// 辅助光源
	ADirectionalLight* DirectionalLight1;
	// 主光源
	ADirectionalLight* DirectionalLight2;

};
