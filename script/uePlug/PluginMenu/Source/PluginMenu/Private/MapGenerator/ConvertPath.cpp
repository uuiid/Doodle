#include "MapGenerator/ConvertPath.h"

#include "FileManager.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FConvertPath"

FString FConvertPath::ToRelativePath(FString &AbsolutePath)
{
	FString RelativePath;

	RelativePath = AbsolutePath;

	if (IFileManager::Get().DirectoryExists(*AbsolutePath))
		if (AbsolutePath.Contains(FPaths::ProjectContentDir()))
			RelativePath = AbsolutePath.Replace(*FPaths::ProjectContentDir(), TEXT("/Game/"));

	return RelativePath;
}

FString FConvertPath::ToAbsolutePath(FString &RelativePath)
{
	FString AbsolutePath;

	AbsolutePath = RelativePath;

	if (RelativePath.StartsWith("/Game/"))
		AbsolutePath = FPaths::ProjectContentDir() + RelativePath.RightChop(6);

	return AbsolutePath;
}

FString FConvertPath::ToRelativePath(FString &AbsoluteFilePath, bool WithExt)
{
	FString FilePath, FileName, RelativePath;

	RelativePath = AbsoluteFilePath;

	if (IFileManager::Get().FileExists(*AbsoluteFilePath))
	{
		if (AbsoluteFilePath.Contains(FPaths::ProjectContentDir()))
		{
			FString FilePath = FPaths::GetPath(AbsoluteFilePath) + "/";
			if (WithExt)
				FileName = FPaths::GetCleanFilename(AbsoluteFilePath);
			else
				FileName = FPaths::GetBaseFilename(AbsoluteFilePath);

			RelativePath = FilePath.Replace(*FPaths::ProjectContentDir(), TEXT("/Game/")) + FileName;
		}
	}

	return RelativePath;
}

FString FConvertPath::ToAbsolutePath(FString &RelativeFilePath, bool WithExt, FString &Ext)
{
	FString AbsolutePath = RelativeFilePath;
	if (RelativeFilePath.StartsWith("/Game/"))
	{
		if (WithExt)
			AbsolutePath = FPaths::ProjectContentDir() + RelativeFilePath.RightChop(6) + "." + Ext;
		else
			AbsolutePath = FPaths::ProjectContentDir() + RelativeFilePath.RightChop(6);
	}
	return AbsolutePath;
}

FString FConvertPath::GetPackageName(FString &Package)
{
	TArray<FString> PackageSplit;
	Package.ParseIntoArray(PackageSplit, TEXT("/"), true);

	FString Name = PackageSplit[PackageSplit.Num() - 1];

	return Name;
}

FString FConvertPath::GetPackagePath(FString &Package)
{
	TArray<FString> PackageSplit;
	Package.ParseIntoArray(PackageSplit, TEXT("/"), true);

	FString Path = "";
	if (PackageSplit.Num() > 1)
		Path = Package.LeftChop(PackageSplit[PackageSplit.Num() - 1].Len() + 1);

	return Path;
}

#undef LOCTEXT_NAMESPACE