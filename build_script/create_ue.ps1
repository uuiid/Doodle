$OutputEncoding = [System.Text.Encoding]::UTF8

. "$PSScriptRoot\set_main_var.ps1"

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force
 
$DoodleBuildRoot = Convert-Path "$DoodleGitRoot/build/UE"
$UEVersion = "5.5"
if (-not (Test-Path $DoodleBuildRoot)) {
  New-Item -ItemType Directory -Path $DoodleBuildRoot | Out-Null
}

$Tags = git tag --sort=-v:refname;
# 去除 前缀 v
$Tags = $Tags | ForEach-Object { $_ -replace "v", "" }
$DoodleVersion = $Tags[0]
# 4.2.233 -> 233
$DoodleVersionNum = $DoodleVersion -split "\." | Select-Object -Last 1

Compress-UEPlugins -UEVersion $UEVersion -MyVersion $DoodleVersionNum -DoodleGitRoot $DoodleGitRoot -OutPath $DoodleBuildRoot

$target = "\\$DoodleIp\Dev\kitsu_data\ue_plugins"
&Robocopy "$DoodleBuildRoot" "$target" /MIR /w:1 /NDL /NFL
 
$headers = @{
  "Authorization" = "Bearer $KitsuCookies"
  "Content-Type"  = "application/json"
}
Invoke-WebRequest -Uri "http://$DoodleIp/api/ue-plugins/version" -Method Post -Headers $headers -Body "$UEVersion.$DoodleVersionNum"
