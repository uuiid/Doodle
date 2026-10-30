#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"

namespace DoodleKitsuClient
{
	struct FDoodleListenerPortValue
	{
		uint16 Port = 0;
	};

	using FDoodleListenerPortResult = TValueOrError<FDoodleListenerPortValue, FString>;

	DOODLELIGHTINGEDITOR_API FDoodleListenerPortResult ReadListenerPort();

	DOODLELIGHTINGEDITOR_API FString GetListenerPortShmName();
}
