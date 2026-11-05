#include "MetaHumanFaceCommandlet.h"

#include "DoodleMetaHumanFaceModule.h"
#include "MetaHumanFaceCaptureDataBuilder.h"
#include "MetaHumanFaceCommandLine.h"
#include "MetaHumanFaceCurveWriter.h"
#include "MetaHumanFaceSolver.h"

UMetaHumanFaceCommandlet::UMetaHumanFaceCommandlet() {
  IsClient = false;
  IsServer = false;
  IsEditor = true;
  LogToConsole = true;
  ShowErrorCount = true;
  UseCommandletResultAsExitCode = true;
  HelpDescription = TEXT("Converts one frame sequence to MetaHuman face curves");
  HelpUsage = TEXT("-FramesDir=<dir> -OutDir=<dir> [-FPS=num/den] [-Range=start:count]");
  HelpParamNames.Add(TEXT("FramesDir"));
  HelpParamNames.Add(TEXT("OutDir"));
  HelpParamNames.Add(TEXT("FPS"));
  HelpParamNames.Add(TEXT("Range"));
  HelpParamDescriptions.Add(TEXT("Directory holding the frame sequence"));
  HelpParamDescriptions.Add(TEXT("Directory receiving the curve output"));
  HelpParamDescriptions.Add(TEXT("Frame rate as a rational number, default 24000/1"));
  HelpParamDescriptions.Add(TEXT("First frame and frame count, default covers the whole sequence"));
}

int32 UMetaHumanFaceCommandlet::Main(const FString& Params) {
  return 0;
}

FMetaHumanFaceJobResult UMetaHumanFaceCommandlet::Run(const FMetaHumanFaceJobRequest& InRequest) {
  return {};
}
