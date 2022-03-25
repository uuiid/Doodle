#include "UDoodleImportUilt.h"

UDoodleImportUilt *UDoodleImportUilt::Get()
{
    TArray<UClass *> ImportUiltClasses;
    GetDerivedClasses(UDoodleImportUilt::StaticClass(),
                      ImportUiltClasses);
    int32 NumClasses = ImportUiltClasses.Num();
    if (NumClasses > 0)
    {
        return Cast<UDoodleImportUilt>(ImportUiltClasses[NumClasses - 1]->GetDefaultObject());
    }
    return nullptr;
}