
# Source - https://stackoverflow.com/a/2124759
# Posted by Andy S, modified by community. See post 'Timeline' for change history
# Retrieved 2025-11-11, License - CC BY-SA 4.0

Push-Location 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\'
cmd /c "vcvars64.bat&set" |
ForEach-Object {
  if ($_ -match "=") {
    $v = $_.split("="); set-item -force -path "ENV:\$($v[0])"  -value "$($v[1])"
  }
}
Pop-Location
write-host "`nVisual Studio 2010 Command Prompt variables set." -ForegroundColor Yellow
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


Write-Host "First argument: $($args[1..($args.Count -1)])"
if ($args.Length -ne 0) {
  $CmakeProcess = Start-Process -FilePath $args[0] -ArgumentList $args[1..($args.Count - 1)] -WorkingDirectory $DoodleRoot -NoNewWindow -PassThru
  while ($CmakeProcess.HasExited -eq $false) {
    $CmakeProcess.Refresh()
    # 等待 0.1s
    Start-Sleep -Milliseconds 100
  }
}

