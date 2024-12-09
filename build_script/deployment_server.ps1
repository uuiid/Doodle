#Set-Item WSMan:\localhost\Client\TrustedHosts -Value '*'
#Set-Item WSMan:\localhost\Client\Auth\Basic -Value True
#Set-Item WSMan:\localhost\Service\Auth\Basic -Value True
#Set-Item WSMan:\localhost\Client\AllowUnencrypted -Value True
#Set-Item WSMan:\localhost\Service\AllowUnencrypted -Value True
# Get-ExecutionPolicy
# Set-ExecutionPolicy RemoteSigned

$OutputEncoding = [System.Text.Encoding]::UTF8

$DoodleSource = Convert-Path "$PSScriptRoot/../build/Ninja_release/_CPack_Packages/win64/ZIP"
$DoodleName = (Get-ChildItem $DoodleSource -Directory)[0].Name

Write-Host "从 "$DoodleSource\$DoodleName\bin" 复制到 192.168.40.181\tmp"
&robocopy "$DoodleSource\$DoodleName\bin" "\\192.168.40.181\tmp\bin" /MIR
&robocopy "E:\source\kitsu\dist" "\\192.168.40.181\tmp\dist" /MIR

Copy-Item "$DoodleSource/$DoodleName.zip" -Destination "\\192.168.40.181\tmp\dist"
Set-Content -Path "\\192.168.40.181\tmp\dist\version.txt" -Value $DoodleName.Split("-")[1]

Copy-Item "E:\source\doodle\dist\doodle.exe" -Destination "\\192.168.40.181\tmp\dist"

$RootPassword = ConvertTo-SecureString "root" -AsPlainText -Force
$Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList auto_light,$RootPassword
#Enter-PSSession -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic
Invoke-Command -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic -ScriptBlock {
#    Compare-Object -ReferenceObject (Get-Content -Path "D:\tmp\bin\file_association_http.exe") -DifferenceObject (Get-Content -Path "D:\kitsu\bin\file_association_http.exe")
    $Target = "D:\kitsu"
    $Tmp = "D:\tmp"
    if ((Get-FileHash "$Target\bin\file_association_http.exe").Hash -ne (Get-FileHash "$Tmp\bin\file_association_http.exe").Hash)
    {
        Stop-Service -Force -Name doodle_kitsu_supplement
        &robocopy "$Tmp\bin" "$Target\bin" /MIR
        Start-Service -Name doodle_kitsu_supplement
    }
    &robocopy "$Tmp\dist" "$Target\dist" /MIR
}

