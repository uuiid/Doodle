param (
    [switch]$CopyServer,
    [switch]$BuildKitsu
)

$OutputEncoding = [System.Text.Encoding]::UTF8

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force
$DoodleOut = Convert-Path "$PSScriptRoot\..\build\pack"
Initialize-Doodle -OutPath $DoodleOut -BuildKitsu:$BuildKitsu