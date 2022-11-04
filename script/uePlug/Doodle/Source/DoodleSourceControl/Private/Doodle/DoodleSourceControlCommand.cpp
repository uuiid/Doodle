#include "DoodleSourceControlCommand.h"

FDoodleSourceControlCommand::FDoodleSourceControlCommand(
    const TSharedRef<class ISourceControlOperation, ESPMode::ThreadSafe>& InOperation,
    const TSharedRef<class IDoodleSourceControlWorker, ESPMode::ThreadSafe>& InWorker,
    const FSourceControlOperationComplete& InOperationCompleteDelegate = FSourceControlOperationComplete()
) : Operation(InOperation), Worker(InWorker), OperationCompleteDelegate(InOperationCompleteDelegate) {
}

bool FDoodleSourceControlCommand::DoWork() {
}

void FDoodleSourceControlCommand::Abandon() {
}

void FDoodleSourceControlCommand::DoThreadedWork() {
}

ECommandResult::Type FDoodleSourceControlCommand::ReturnResults() {
}