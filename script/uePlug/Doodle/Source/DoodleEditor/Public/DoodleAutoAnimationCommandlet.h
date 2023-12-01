// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "LevelSequence.h"

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
	void OnCreateSequence();
	void OnBuildSequence();

	TSharedPtr<FJsonObject> JsonObject;
	ULevelSequence* TheLevelSequence;
	FString DestinationPath;
	TArray<UAssetImportTask*> ImportTasks;
	FString SequencePath;
	FString ImportPath;
	FName MapName;
	FString MoviePipelineConfigPath;

	FFrameNumber L_Start{1001};
	FFrameNumber L_End{1200};
};
