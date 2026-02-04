// MIT License


using UnrealBuildTool;
using System.IO;

public class DNACalibLibExt : ModuleRules
{
	public DNACalibLibExt(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		bUseUnity = false;
		bEnableExceptions = true;
		bUseRTTI = true; // Needed by CLI11 library

 		PrivateDependencyModuleNames.Add("DNACalibLib");
	}
}
