#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "DNACharacterDiagnoseCommandlet.generated.h"

UCLASS()
class UDNACharacterDiagnoseCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UDNACharacterDiagnoseCommandlet();

	virtual int32 Main(const FString& Params) override;
};
