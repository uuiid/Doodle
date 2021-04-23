#include "DoodleCommandlet.h"

UDoodleAssCreateCommandlet::UDoodleAssCreateCommandlet(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
}
int32 UDoodleAssCreateCommandlet::Main(const FString &Params)
{
    bool bSuccess = false;

    const TCHAR *ParamStr = *Params;
    ParseCommandLine(ParamStr, CmdLineTokens, CmdLineSwitches);
    for (auto i : CmdLineTokens)
    {
        UE_LOG(LogTemp, Log, TEXT("CmdLineTokens: %s"), *(i));
    }
    for (auto i : CmdLineSwitches)
    {
        UE_LOG(LogTemp, Log, TEXT("CmdLineSwitches: %s"), *(i));
    }
    FPlatformProcess::Sleep(1.0f);
    return 0;
}
