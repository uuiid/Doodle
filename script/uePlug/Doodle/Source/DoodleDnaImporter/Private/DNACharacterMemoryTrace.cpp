
#include "DNACharacterMemoryTrace.h"

#include "DNACharacterImporterModule.h"

#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"
#include "Logging/LogMacros.h"

namespace
{
	struct FMemoryValues
	{
		uint64 UsedPhysical = 0;
		uint64 PeakPhysical = 0;
		uint64 UsedVirtual = 0;
		uint64 PeakVirtual = 0;
	};

	FMemoryValues ReadMemory()
	{
		const FPlatformMemoryStats Stats = FPlatformMemory::GetStats();
		FMemoryValues Values;
		Values.UsedPhysical = Stats.UsedPhysical;
		Values.PeakPhysical = Stats.PeakUsedPhysical;
		Values.UsedVirtual = Stats.UsedVirtual;
		Values.PeakVirtual = Stats.PeakUsedVirtual;
		return Values;
	}
}

void FDNAMemoryTrace::Reset()
{
	const FMemoryValues Baseline = ReadMemory();
	BaselineUsedPhysical = Baseline.UsedPhysical;
	BaselinePeakPhysical = Baseline.PeakPhysical;
	BaselineUsedVirtual = Baseline.UsedVirtual;
	BaselinePeakVirtual = Baseline.PeakVirtual;
	CurrentUsedPhysical = BaselineUsedPhysical;
	CurrentPeakPhysical = BaselinePeakPhysical;
	CurrentUsedVirtual = BaselineUsedVirtual;
	CurrentPeakVirtual = BaselinePeakVirtual;
	Samples.Reset();
	StartTime = FPlatformTime::Seconds();
	LastSampleTime = StartTime;
	bStarted = true;
}

void FDNAMemoryTrace::Checkpoint(const FString& Label)
{
	if (!bStarted)
	{
		Reset();
	}

	FDNAMemorySample Sample;
	Sample.Label = Label;
	const FMemoryValues Current = ReadMemory();
	Sample.UsedPhysical = Current.UsedPhysical;
	Sample.PeakUsedPhysical = Current.PeakPhysical;
	Sample.UsedVirtual = Current.UsedVirtual;
	Sample.PeakUsedVirtual = Current.PeakVirtual;

	const double Now = FPlatformTime::Seconds();
	Sample.Seconds = Now - StartTime;
	Sample.DeltaSeconds = Now - LastSampleTime;
	LastSampleTime = Now;

	CurrentUsedPhysical = Sample.UsedPhysical;
	CurrentPeakPhysical = Sample.PeakUsedPhysical;
	CurrentUsedVirtual = Sample.UsedVirtual;
	CurrentPeakVirtual = Sample.PeakUsedVirtual;

	Samples.Add(MoveTemp(Sample));
}

uint64 FDNAMemoryTrace::GetPhysicalGrowth() const
{
	return CurrentUsedPhysical > BaselineUsedPhysical ? CurrentUsedPhysical - BaselineUsedPhysical : 0;
}

uint64 FDNAMemoryTrace::GetPeakGrowth() const
{
	return CurrentPeakPhysical > BaselinePeakPhysical ? CurrentPeakPhysical - BaselinePeakPhysical : 0;
}

double FDNAMemoryTrace::GetElapsedSeconds() const
{
	return bStarted ? FPlatformTime::Seconds() - StartTime : 0.0;
}

FString FDNAMemoryTrace::FormatBytes(uint64 Bytes)
{
	const double GigaBytes = static_cast<double>(Bytes) / (1024.0 * 1024.0 * 1024.0);
	if (GigaBytes >= 1.0)
	{
		return FString::Printf(TEXT("%8.3f GB"), GigaBytes);
	}
	const double MegaBytes = static_cast<double>(Bytes) / (1024.0 * 1024.0);
	return FString::Printf(TEXT("%8.1f MB"), MegaBytes);
}

FString FDNAMemoryTrace::ToString() const
{
	FString Out;
	Out += FString::Printf(TEXT("  baseline: used %s   peak %s\n"),
		*FormatBytes(BaselineUsedPhysical), *FormatBytes(BaselinePeakPhysical));

	for (const FDNAMemorySample& Sample : Samples)
	{
		Out += FString::Printf(TEXT("  [%7.2fs +%6.2fs] used %s  peak %s   %s\n"),
			Sample.Seconds,
			Sample.DeltaSeconds,
			*FormatBytes(Sample.UsedPhysical),
			*FormatBytes(Sample.PeakUsedPhysical),
			*Sample.Label);
	}

	Out += FString::Printf(TEXT("  growth: used %s   peak %s\n"),
		*FormatBytes(GetPhysicalGrowth()), *FormatBytes(GetPeakGrowth()));
	return Out;
}

void FDNAMemoryTrace::Log(const FString& Title) const
{
	UE_LOG(LogDoodleDnaImporter, Display, TEXT("=== memory trace: %s ==="), *Title);
	TArray<FString> Lines;
	ToString().ParseIntoArrayLines(Lines, true);
	for (const FString& Line : Lines)
	{
		UE_LOG(LogDoodleDnaImporter, Display, TEXT("%s"), *Line);
	}
}
