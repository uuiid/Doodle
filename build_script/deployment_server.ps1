param (
    [switch]$CopyServer,
    [switch]$BuildKitsu,
    [switch]$CreateUEPlugins
)

$OutputEncoding = [System.Text.Encoding]::UTF8

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force
$KitsuCookies = $env:KITSU_COOKIES;
$DoodleOut = Convert-Path "$PSScriptRoot/../build/pack"
Initialize-Doodle -OutPath $DoodleOut -BuildKitsu:$BuildKitsu -CreateUEPlugins:$CreateUEPlugins

&robocopy "$DoodleOut" "\\192.168.40.181\Dev\tmp" /s | Out-Null
&robocopy "$DoodleOut\dist\assets" "\\192.168.40.181\Dev\tmp\dist\assets" /MIR | Out-Null

$RootPassword = ConvertTo-SecureString "root" -AsPlainText -Force
$Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList administrator, $RootPassword
#Enter-PSSession -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic

Invoke-Command -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic -ScriptBlock {
    #    Compare-Object -ReferenceObject (Get-Content -Path "D:\tmp\bin\file_association_http.exe") -DifferenceObject (Get-Content -Path "D:\kitsu\bin\file_association_http.exe")
    $Target = "D:\kitsu"
    $Tmp = "D:\tmp"
    $timestamp = Get-Date -Format o | ForEach-Object { $_ -replace ":", "." }
    $LogPath = "$env:TEMP\build_$timestamp.log"
    &robocopy "$Tmp\dist" "$Target\dist" /MIR /unilog+:$LogPath /w:1
}

if ($CopyServer) {
    Invoke-Command -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic -ScriptBlock {
        $Kitsu_Ip = "192.168.40.181"
        Write-Host "使用 Kitsu http://$Kitsu_Ip/api/doodle/stop-server 进行更新"
        #    Compare-Object -ReferenceObject (Get-Content -Path "D:\tmp\bin\file_association_http.exe") -DifferenceObject (Get-Content -Path "D:\kitsu\bin\file_association_http.exe") $Using:CopyServer

        $headers = @{
            "Authorization" = "Bearer $Using:KitsuCookies"
        }

        Invoke-WebRequest -Uri "http://$Kitsu_Ip/api/doodle/stop-server" -Method Post -Headers $headers 
        $Target = "D:"
        $Tmp = "D:\tmp"
        $timestamp = Get-Date -Format o | ForEach-Object { $_ -replace ":", "." }
        $LogPath = "$env:TEMP\build_$timestamp.log"
        # 找到停止的服务
        $StopedServers = ""
        # Get-EventLog -LogName Application -Source nssm -Before ((Get-Date).AddMonths(-3)) | Remove-EventLog -Confirm:$false
        foreach ($server in (Get-Service "doodle_kitsu_*" | Sort-Object Status)) {
            if ($server.Status -eq "Stopped") {
                $StopedServers = $server.Name
                if ((Get-FileHash "$Target\$StopedServers\bin\doodle_kitsu_supplement.exe").Hash -ne (Get-FileHash "$Tmp\bin\doodle_kitsu_supplement.exe").Hash) {
                    Write-Host "更新服务 $($server.Name)"
                    &robocopy "$Tmp\bin" "$Target\$($server.Name)\bin" /MIR /unilog+:$LogPath /w:1 | Out-Null
                    Start-Service -InputObject $server
                    Set-Service -Name $server.Name -StartupType Automatic
                }
            }
            else {
                if ((Get-FileHash "$Target\$StopedServers\bin\doodle_kitsu_supplement.exe").Hash -ne (Get-FileHash "$Tmp\bin\doodle_kitsu_supplement.exe").Hash) {
                    Write-Host "服务 $($server.Name) 未停止，设置为手动启动"
                    Set-Service -Name $server.Name -StartupType Manual 
                }
            }
        }
    }
}
 


