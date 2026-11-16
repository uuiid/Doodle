
#include "DNACharacterImporterModule.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDoodleDnaImporter);

void FDNACharacterImporterModule::StartupModule()
{
}

void FDNACharacterImporterModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FDNACharacterImporterModule, DoodleDnaImporter)
