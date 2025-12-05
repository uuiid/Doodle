param (
    [switch]$CopyServer,
    [switch]$BuildKitsu,
    [switch]$CreateUEPlugins
)

$OutputEncoding = [System.Text.Encoding]::UTF8

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force
$DoodleOut = Convert-Path "$PSScriptRoot/../build/pack"
Initialize-Doodle -OutPath $DoodleOut -BuildKitsu:$BuildKitsu -CreateUEPlugins:$CreateUEPlugins

&Robocopy "$DoodleOut" "\\192.168.40.181\Dev\tmp" /MIR /xf "*.zip" /w:1 
&Robocopy "$DoodleOut" "\\192.168.40.181\Dev\tmp" /s  /w:1 


$NewSession = New-ServerPSSession

$KitsuCookies = (Get-ItemProperty -Path HKLM:\SOFTWARE\Doodle -Name kitsu_cookies).kitsu_cookies;
Invoke-Command -Session $NewSession -ScriptBlock {
    $Target = "D:\kitsu"
    $Tmp = "D:\tmp"
    $timestamp = Get-Date -Format o | ForEach-Object { $_ -replace ":", "." }
    $LogPath = "$env:TEMP\build_$timestamp.log"
    &robocopy "$Tmp\dist" "$Target\dist" /MIR /unilog+:$LogPath /w:1
}


if ($CopyServer) {
    Invoke-Command -Session $NewSession  -ArgumentList $KitsuCookies -ScriptBlock {
        param ($KitsuCookies)
        $Kitsu_Ip = "127.0.0.1"
        Write-Host "使用 Kitsu http://$Kitsu_Ip/api/doodle/stop-server 进行更新"
        #    Compare-Object -ReferenceObject (Get-Content -Path "D:\tmp\bin\file_association_http.exe") -DifferenceObject (Get-Content -Path "D:\kitsu\bin\file_association_http.exe") $Using:CopyServer

        $headers = @{
            "Authorization" = "Bearer $KitsuCookies"
        }

        Invoke-WebRequest -Uri "http://$Kitsu_Ip/api/doodle/stop-server" -Method Post -Headers $headers 
        $Target = "D:"
        $Tmp = "D:\tmp"
        $timestamp = Get-Date -Format o | ForEach-Object { $_ -replace ":", "." }
        $LogPath = "$env:TEMP\build_$timestamp.log"
        # 找到停止的服务
        $UpdataServers = $false
        $ScrversName = ""
        # Get-EventLog -LogName Application -Source nssm -Before ((Get-Date).AddMonths(-3)) | Remove-EventLog -Confirm:$false
        foreach ($server in (Get-Service "doodle_kitsu_*" | Sort-Object Status)) {
            if ($server.Status -eq "Stopped") {
                if ((Get-FileHash "$Target\$($server.Name)\bin\doodle_kitsu_supplement.exe").Hash -ne (Get-FileHash "$Tmp\bin\doodle_kitsu_supplement.exe").Hash) {
                    Write-Host "更新服务 $($server.Name)"
                    &robocopy "$Tmp\bin" "$Target\$($server.Name)\bin" /MIR /unilog+:$LogPath /w:1 | Out-Null
                    Start-Service -InputObject $server
                    Set-Service -Name $server.Name -StartupType Automatic
                    $UpdataServers = $true
                }
            }
            else {
                if ($UpdataServers) {
                    Write-Host "服务 $($server.Name) 未停止 将在 $((Get-Date).AddMinutes(20)) 停止，设置为手动启动"
                    Set-Service -Name $server.Name -StartupType Manual 
                    $ScrversName = $server.Name
                }
            }
        }
    }
}
 


