// Copyright Epic Games, Inc. All Rights Reserved.

#include "BatchRender.h"
#include "MoviePipelineQueue.h"
#include "MoviePipelineQueueSubsystem.h"
#include "MoviePipelineConfigBase.h"
#include "MovieRenderPipelineSettings.h"
#include "MoviePipelineDeferredPasses.h"
#include "MoviePipelineAntiAliasingSetting.h"
#include "MoviePipelineImageSequenceOutput.h"
#include <DoodleNetWork.h>

#define LOCTEXT_NAMESPACE "FBatchRenderModule"



void FBatchRenderModule::StartupModule()
{
	//DoodleNetWork = MakeShared<class FDoodleNetwork>();
	//PipelineSubsystem = GEngine->GetEngineSubsystem<UBatchRenderQueueEngineSubsystem>();
	//PipelineSubsystem->OnBatchRenderFinished.AddRaw(this, &FBatchRenderModule::OnQueueFinished);
}
void FBatchRenderModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FBatchRenderModule,BatchRender)
#undef LOCTEXT_NAMESPACE
