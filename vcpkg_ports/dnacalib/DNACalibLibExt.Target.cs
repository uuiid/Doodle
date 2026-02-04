// MIT License

// Copyright (c) 2022 Autodesk, Inc.

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

using UnrealBuildTool;
using System;
using System.IO;
using EpicGames.Core;
using System.Collections.Generic;
using System.Runtime.CompilerServices;


// [SupportedPlatforms("Win64", "Linux")]
public class DNACalibLibExtTarget : TargetRules
{
	/// <summary>
	/// Finds the innermost parent directory with the provided name. Search is case insensitive.
	/// </summary>
	string InnermostParentDirectoryPathWithName(string ParentName, string CurrentPath)
	{
		DirectoryInfo ParentInfo = Directory.GetParent(CurrentPath);

		if (ParentInfo == null)
		{
			throw new DirectoryNotFoundException("Could not find parent folder '" + ParentName + "'");
		}

		// Case-insensitive check of the parent folder name.
		if (ParentInfo.Name.ToLower() == ParentName.ToLower())
		{
			return ParentInfo.ToString();
		}

		return InnermostParentDirectoryPathWithName(ParentName, ParentInfo.ToString());
	}

	/// <summary>
	/// Returns the path to this .cs file.
	/// </summary>
	string GetCallerFilePath([CallerFilePath] string CallerFilePath = "")
	{
		if (CallerFilePath.Length == 0)
		{
			throw new FileNotFoundException("Could not find the path of our .cs file");
		}

		return CallerFilePath;
	}

	public DNACalibLibExtTarget(TargetInfo Target) : base(Target)
	{
		Init(Target, "2020");
	}

	
	private void Init(TargetInfo Target, string InMayaVersionString)
	{
		Type = TargetType.Program;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		bShouldCompileAsDLL = true;
		LinkType = TargetLinkType.Monolithic;
		SolutionDirectory = "Programs/LiveLink";
		string MllName = "DNACalibLibExt";
		LaunchModuleName = MllName;

		ReadOnlyBuildVersion Version = ReadOnlyBuildVersion.Current;
		string UEVersion = Version.MajorVersion.ToString()+"_"+Version.MinorVersion.ToString();
		Name             = MllName + "_" + UEVersion;

		// 我们只需要很少使用这个插件的引擎
		bBuildDeveloperTools = false;
		// bUseMallocProfiler = false;
		bBuildWithEditorOnlyData = true;
		bCompileAgainstEngine = false;
		bCompileAgainstCoreUObject = true;
		// bCompileAgainstApplicationCore = false;
		bCompileAgainstApplicationCore = true;
		bCompileICU = false;
		bHasExports = true;
		

		bBuildInSolutionByDefault = false;
		bUseAdaptiveUnityBuild = false;

		// 该.cs文件必须位于该程序的源文件夹内。我们随后利用这些数据查找其他关键目录。
		string TargetFilePath = GetCallerFilePath();

		// 我们需要避免由于在不存在的文件夹中查找 EngineDir而无法加载DLL。
    // 通过将其构建在与引擎相同的目录中，它将假设引擎与程序位于同一目录中，
    // 并且由于此文件夹始终存在，因此它不会在EngineDir中的检查中失败。 

		// 寻找引擎必要目录
		string ProgramsDir = InnermostParentDirectoryPathWithName("Programs", TargetFilePath);
		string SourceDir = InnermostParentDirectoryPathWithName("Source", ProgramsDir);
		string EngineDir = InnermostParentDirectoryPathWithName("Engine", SourceDir);

		// 默认的二进制文件路径被假定为“源”文件夹的兄弟路径。
		string DefaultBinDir = Path.GetFullPath(Path.Combine(SourceDir, "..", "Binaries", Platform.ToString()));

		// 我们假设引擎exe驻留在engine/Binaries/[Platform]中
		string EngineBinariesDir = Path.Combine(EngineDir, "Binaries", Platform.ToString(), "Maya");

		// 现在我们计算默认输出目录和引擎二进制文件之间的相对路径，
		// 以便强制此程序的输出与引擎位于同一文件夹中。
		ExeBinariesSubFolder = (new DirectoryReference(EngineBinariesDir)).MakeRelativeTo(new DirectoryReference(DefaultBinDir));

		// 设置此项是必要的，因为我们是在Restricted之外创建二进制文件。
		bLegalToDistributeBinary = true;

		// 我们仍然需要复制资源，因此此时我们最好将文件复制到默认Binaries文件夹的位置。MayaRealLiveLinkPlugin.xml将不知道文件是如何到达那里的。

		// 添加一个后构建步骤，将输出复制到扩展名为.mll的文件中
		string OutputName = Name;
		if (Target.Configuration != UnrealTargetConfiguration.Development)
		{
			OutputName = string.Format("{0}-{1}-{2}", OutputName, Target.Platform, Target.Configuration);
			MllName = string.Format("{0}-{1}-{2}", MllName, Target.Platform, Target.Configuration);
		}

		string PostBuildBinDir = Path.Combine(DefaultBinDir, "Maya");

		bool IsLinux = System.Environment.OSVersion.Platform.ToString() == "Unix";

		if (Target.Platform == UnrealTargetPlatform.Linux) {
			OutputName = "lib" + OutputName;
			MllName = "lib" + MllName;
		}

		// 使用正确版本的UE修复脚本
		DirectoryInfo SourcePath = Directory.GetParent(TargetFilePath);

		// Copy binaries
		PostBuildSteps.Add(string.Format("echo Copying {0} to {1}...", EngineBinariesDir, PostBuildBinDir));

		if (!IsLinux)
		{
			PostBuildSteps.Add(string.Format("xcopy /y /i /v \"{0}\\{1}.*\" \"{2}\\{3}.*\" 1>nul", EngineBinariesDir, OutputName, PostBuildBinDir, OutputName));
		}
		else
		{
			PostBuildSteps.Add(string.Format("mkdir -p \"{2}\" && rm -f {2}/{1}* && cp \"{0}\"/\"{1}\".* \"{2}\" 1>nul", EngineBinariesDir, OutputName, PostBuildBinDir));
		}

		string SourceFileName = Path.Combine(PostBuildBinDir, OutputName);
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Rename dll as mll
			string OutputFileName = Path.Combine(PostBuildBinDir, MllName + "_" + UEVersion);

			PostBuildSteps.Add(string.Format("echo Renaming {0}.dll to {1}.mll...", SourceFileName, OutputFileName));
			PostBuildSteps.Add(string.Format("move /Y \"{0}.dll\" \"{1}.mll\" 1>nul", SourceFileName, OutputFileName));
		} else if (Target.Configuration != UnrealTargetConfiguration.Development)
		{
			// For Linux debug builds, create symbolic link when library name is different
			string OutputFileName = Path.Combine(PostBuildBinDir, "lib" + Name);

			PostBuildSteps.Add(string.Format("echo \"Creating symbolic links {1}.* to {0}.*...\"", SourceFileName, OutputFileName));
			PostBuildSteps.Add(string.Format("ln -sf \"{0}.{2}\" \"{1}.{2}\" 1>nul", SourceFileName, OutputFileName,"so"));
			PostBuildSteps.Add(string.Format("ln -sf \"{0}.{2}\" \"{1}.{2}\" 1>nul", SourceFileName, OutputFileName,"debug"));
			PostBuildSteps.Add(string.Format("ln -sf \"{0}.{2}\" \"{1}.{2}\" 1>nul", SourceFileName, OutputFileName,"sym"));
		}
	}
}
