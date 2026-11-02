#include "DoodleLightingTypes.h"

#include "Misc/Paths.h"

namespace DoodleLightingProtocol
{
	FString MakeCandidateFramesDir(const FString& InOutDir, const FString& InCandidateId)
	{
		if (InOutDir.IsEmpty() || InCandidateId.IsEmpty())
		{
			return FString();
		}
		return FPaths::Combine(InOutDir, InCandidateId, TEXT("MRQ")) + TEXT("/");
	}

	FString MakeCandidateMoviePath(const FString& InOutDir, const FString& InCandidateId)
	{
		if (InOutDir.IsEmpty() || InCandidateId.IsEmpty())
		{
			return FString();
		}
		return FPaths::Combine(InOutDir, FString::Printf(TEXT("%s.mp4"), *InCandidateId));
	}
}
