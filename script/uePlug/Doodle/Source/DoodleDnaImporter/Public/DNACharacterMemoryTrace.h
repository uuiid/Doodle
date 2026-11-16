
#pragma once

#include "CoreMinimal.h"

struct DOODLEDNAIMPORTER_API FDNAMemorySample
{
	FString Label;
	double Seconds = 0.0;
	double DeltaSeconds = 0.0;
	uint64 UsedPhysical = 0;
	uint64 PeakUsedPhysical = 0;
	uint64 UsedVirtual = 0;
	uint64 PeakUsedVirtual = 0;
};

class DOODLEDNAIMPORTER_API FDNAMemoryTrace
{
public:
	void Reset();

	void Checkpoint(const FString& Label);

	bool IsStarted() const { return bStarted; }

	uint64 GetBaselineUsedPhysical() const { return BaselineUsedPhysical; }
	uint64 GetBaselinePeakPhysical() const { return BaselinePeakPhysical; }
	uint64 GetCurrentUsedPhysical() const { return CurrentUsedPhysical; }
	uint64 GetCurrentPeakPhysical() const { return CurrentPeakPhysical; }
	uint64 GetCurrentUsedVirtual() const { return CurrentUsedVirtual; }
	uint64 GetCurrentPeakVirtual() const { return CurrentPeakVirtual; }

	uint64 GetPhysicalGrowth() const;

	uint64 GetPeakGrowth() const;

	double GetElapsedSeconds() const;

	const TArray<FDNAMemorySample>& GetSamples() const { return Samples; }

	FString ToString() const;

	void Log(const FString& Title) const;

	static FString FormatBytes(uint64 Bytes);

private:
	bool bStarted = false;
	double StartTime = 0.0;
	double LastSampleTime = 0.0;
	uint64 BaselineUsedPhysical = 0;
	uint64 BaselinePeakPhysical = 0;
	uint64 BaselineUsedVirtual = 0;
	uint64 BaselinePeakVirtual = 0;
	uint64 CurrentUsedPhysical = 0;
	uint64 CurrentPeakPhysical = 0;
	uint64 CurrentUsedVirtual = 0;
	uint64 CurrentPeakVirtual = 0;
	TArray<FDNAMemorySample> Samples;
};
