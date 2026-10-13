
# Source - https://stackoverflow.com/a/2124759
# Posted by Andy S, modified by community. See post 'Timeline' for change history
# Retrieved 2025-11-11, License - CC BY-SA 4.0

# 调用 vcvars64.bat 并把它的环境变量导入当前会话。
#
# 三个坑，都在这一行里躲开：
#  1. 路径含空格 —— 某些环境（受限 shell、CI）下 cmd /c 的分词会把带引号的路径拆错，
#     报 "不是内部或外部命令"。改用 8.3 短路径（无空格），由 FileSystemObject 运行时解析，
#     避免把某台机器的 8.3 名写死。
#  2. 环境变量值里也可能含 '='（如 __VSCMD_* / Flag 类变量），
#     用 IndexOf('=') 定位首个分隔符，只切一次，避免值被截断。
#  3. 不要把 cmd 的输出接进 PowerShell 管道 —— 受限环境下会触发
#     "StandardOutputEncoding is only supported when standard output is redirected" 失败。
#     改为把 `set` 的输出重定向到临时文件，再逐行读入。
$VCVARS_DIR = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build'
try {
  $fso = New-Object -ComObject Scripting.FileSystemObject
  $VCVARS_DIR = $fso.GetFolder($VCVARS_DIR).ShortPath
  [void][Runtime.InteropServices.Marshal]::ReleaseComObject($fso)
} catch {
  Write-Warning "无法解析 vcvars 短路径，改用长路径：$_"
}

$VCVARS_DUMP = [System.IO.Path]::GetTempFileName()
try {
  cmd /c "`"$VCVARS_DIR\vcvars64.bat`" >nul 2>&1 && set > `"$VCVARS_DUMP`""
  foreach ($line in [System.IO.File]::ReadAllLines($VCVARS_DUMP)) {
    $idx = $line.IndexOf('=')
    if ($idx -gt 0) {
      Set-Item -Force -Path "ENV:\$($line.Substring(0, $idx))" -Value $line.Substring($idx + 1)
    }
  }
} finally {
  Remove-Item -Force -ErrorAction SilentlyContinue $VCVARS_DUMP
}
write-host "`nVisual Studio Command Prompt variables set." -ForegroundColor Yellow
$env:chcp = 65001
$env:VSLANG = 1033
$DoodleRoot = Convert-Path "$PSScriptRoot/../..";
function Get-ChildProcesses ($ParentProcessId) {
  $filter = "parentprocessid = '$($ParentProcessId)'"
  Get-CIMInstance -ClassName win32_process -filter $filter | Foreach-Object {
    $_
    if ($_.ParentProcessId -ne $_.ProcessId) {
      Get-ChildProcesses $_.ProcessId
    }
  }
}


if ($args.Length -eq 0) {
  Write-Host "No command requested; Visual Studio environment variables are set." -ForegroundColor Yellow
  return
}

Write-Host "Running: $($args[0]) $($args[1..($args.Count - 1)] -join ' ')" -ForegroundColor Cyan
# 用调用运算符而非 Start-Process：Start-Process -PassThru 的 .ExitCode 在受限环境下
# 会返回 $null（进程句柄受限），无法可靠透传退出码。调用运算符的 $LASTEXITCODE 一致可靠。
#
# 调用运算符让子进程继承本 PowerShell 进程的 cwd，所以必须显式固定工作目录：
# cmake --preset 要在源码根目录解析 CMakePresets.json 与 vcpkg 工具链，
# 依赖这个 cwd 的调用方（编辑器任务、dsh-cmake 插件）都可能从别处启动本脚本。
Push-Location $DoodleRoot
try {
  & $args[0] @($args[1..($args.Count - 1)])
} finally {
  Pop-Location
}
# $LASTEXITCODE 只在「被调用的是原生可执行文件」时才会被赋值；若目标不存在，调用运算符
# 抛错、该变量保持上一次的值（可能是 $null）。先判空再取整，避免把失败静默成退出码 0。
if ($null -eq $LASTEXITCODE) {
  Write-Error "命令未产生退出码，视为失败：$($args[0])"
  exit 1
}
exit [int]$LASTEXITCODE

