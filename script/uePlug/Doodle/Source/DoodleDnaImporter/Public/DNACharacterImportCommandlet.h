#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "DNACharacterImportCommandlet.generated.h"

UCLASS()
class UDNACharacterImportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UDNACharacterImportCommandlet();

	virtual int32 Main(const FString& Params) override;
};
