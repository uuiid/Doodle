param (
    [switch]$CopyServer,
    [switch]$BuildKitsu
)

$OutputEncoding = [System.Text.Encoding]::UTF8

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force

$DoodleOut = Convert-Path "$PSScriptRoot/../build/pack"
Initialize-Doodle -OutPath $DoodleOut -BuildKitsu:$BuildKitsu
&robocopy "$DoodleOut" "\\192.168.40.181\Dev\tmp" /s | Out-Null
&robocopy "$DoodleOut\dist\assets" "\\192.168.40.181\Dev\tmp\dist\assets" /MIR | Out-Null

$RootPassword = ConvertTo-SecureString "root" -AsPlainText -Force
$Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList administrator,$RootPassword
#Enter-PSSession -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic
Invoke-Command -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic -ScriptBlock {
    #    Compare-Object -ReferenceObject (Get-Content -Path "D:\tmp\bin\file_association_http.exe") -DifferenceObject (Get-Content -Path "D:\kitsu\bin\file_association_http.exe")
    $Target = "D:\kitsu"
    $Tmp = "D:\tmp"
    $timestamp = Get-Date -Format o | ForEach-Object { $_ -replace ":", "." }
    $LogPath = "$env:TEMP\build_$timestamp.log"
    if ($Using:CopyServer -and ((Get-FileHash "$Target\bin\doodle_kitsu_supplement.exe").Hash -ne (Get-FileHash "$Tmp\bin\doodle_kitsu_supplement.exe").Hash))
    {
        Write-Host "更新服务"
        Stop-Service -Force -Name doodle_kitsu_supplement
        Start-Sleep -Milliseconds 50
        &robocopy "$Tmp\bin" "$Target\bin" /MIR /unilog+:$LogPath /w:1 | Out-Null
        Start-Service -Name doodle_kitsu_supplement
    }
    &robocopy "$Tmp\dist" "$Target\dist" /MIR /unilog+:$LogPath /w:1
}

