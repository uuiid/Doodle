#include "DoodleClassHelper.h"

void UDoodleClassHelper::OnBeginFrame()
{
    //UE_LOG(LogTemp, Warning, TEXT("OnBeginFrame"));
    for (TObjectIterator<UMoviePipeline> It{};It;++It)
    {
        ActiveMoviePipeline = *It;
        if (ActiveMoviePipeline && ActiveMoviePipeline->GetPipelineState() != EMovieRenderPipelineState::Finished)
        {
            ActiveMoviePipeline->OnMoviePipelineWorkFinished().AddUObject(this,&UDoodleClassHelper::OnMoviePipelineWorkFinished);
        }
    }
    FCoreDelegates::OnBeginFrame.RemoveAll(this);
}

void UDoodleClassHelper::OnMoviePipelineWorkFinished(FMoviePipelineOutputData Data)
{
    if (Data.bSuccess)
    {
        for (FMoviePipelineShotOutputData Shot : Data.ShotData)
        {
            TMap<FMoviePipelinePassIdentifier, FMoviePipelineRenderPassOutputData> PassMap = Shot.RenderPassData;
            for (TPair<FMoviePipelinePassIdentifier, FMoviePipelineRenderPassOutputData>& element : PassMap)
            {
                if (element.Key.Name == TEXT("FinalImage"))
                {
                    FMoviePipelineRenderPassOutputData OutputData = element.Value;
                    for (int i = 0;i < OutputData.FilePaths.Num();i++)
                    {
                        FString FileName = FPaths::GetCleanFilename(OutputData.FilePaths[i]);
                        TArray<FString> stringArray;
                        FileName.ParseIntoArray(stringArray, TEXT("."), false);
                        if (stringArray.Num() > 2)
                        {
                            FString NumStr = stringArray[stringArray.Num() - 2];
                            int Num = FCString::Atoi(*NumStr);
                            if (Num <= 1000)
                            {
                                IFileManager::Get().Delete(*OutputData.FilePaths[i]);
                            }
                            //UE_LOG(LogTemp, Warning, TEXT("输出的帧%d"), Num);
                        }
                    }
                }
            }
        }
    }
    if(ActiveMoviePipeline)
    ActiveMoviePipeline->OnMoviePipelineWorkFinished().RemoveAll(this);
}
