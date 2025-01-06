param (
    [switch]$CopyServer
)

$OutputEncoding = [System.Text.Encoding]::UTF8

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force
$DoodleOut = "\\192.168.40.181\tmp"
Initialize-Doodle -OutPath $DoodleOut

$RootPassword = ConvertTo-SecureString "root" -AsPlainText -Force
$Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList auto_light,$RootPassword
#Enter-PSSession -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic
Invoke-Command -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic -ScriptBlock {
    #    Compare-Object -ReferenceObject (Get-Content -Path "D:\tmp\bin\file_association_http.exe") -DifferenceObject (Get-Content -Path "D:\kitsu\bin\file_association_http.exe")
    $Target = "D:\kitsu"
    $Tmp = "D:\tmp"
    if ($Using:CopyServer -and ((Get-FileHash "$Target\bin\doodle_kitsu_supplement.exe").Hash -ne (Get-FileHash "$Tmp\bin\doodle_kitsu_supplement.exe").Hash))
    {
        Write-Host "更新服务"
        Stop-Service -Force -Name doodle_kitsu_supplement
        &robocopy "$Tmp\bin" "$Target\bin" /MIR /np /njh /njs /ns /nc /ndl /fp /ts
        Start-Service -Name doodle_kitsu_supplement
    }
    &robocopy "$Tmp\dist" "$Target\dist" /MIR /np /njh /njs /ns /nc /ndl /fp /ts
}

