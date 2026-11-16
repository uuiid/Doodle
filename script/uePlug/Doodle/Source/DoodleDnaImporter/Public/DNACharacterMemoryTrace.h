// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** One row of the memory trace. */
struct DOODLEDNAIMPORTER_API FDNAMemorySample
{
	FString Label;
	/** Seconds since the trace started. */
	double Seconds = 0.0;
	/** Seconds since the previous sample. */
	double DeltaSeconds = 0.0;
	uint64 UsedPhysical = 0;
	uint64 PeakUsedPhysical = 0;
	uint64 UsedVirtual = 0;
	uint64 PeakUsedVirtual = 0;
};

/**
 * Very small helper that records the process memory at named checkpoints so that a stock
 * import and the low memory import can be compared with real numbers.
 */
class DOODLEDNAIMPORTER_API FDNAMemoryTrace
{
public:
	/** Start a new trace, taking the current memory as the baseline. */
	void Reset();

	/** Record the current memory usage under the given label. */
	void Checkpoint(const FString& Label);

	bool IsStarted() const { return bStarted; }

	uint64 GetBaselineUsedPhysical() const { return BaselineUsedPhysical; }
	uint64 GetBaselinePeakPhysical() const { return BaselinePeakPhysical; }
	uint64 GetCurrentUsedPhysical() const { return CurrentUsedPhysical; }
	uint64 GetCurrentPeakPhysical() const { return CurrentPeakPhysical; }
	uint64 GetCurrentUsedVirtual() const { return CurrentUsedVirtual; }
	uint64 GetCurrentPeakVirtual() const { return CurrentPeakVirtual; }

	/** Physical memory growth of this process since the trace started. */
	uint64 GetPhysicalGrowth() const;

	/** Physical memory peak reached since the trace started (0 when the baseline peak was higher). */
	uint64 GetPeakGrowth() const;

	double GetElapsedSeconds() const;

	const TArray<FDNAMemorySample>& GetSamples() const { return Samples; }

	/** Multi line, human readable table. */
	FString ToString() const;

	/** Log the table through UE_LOG. */
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
