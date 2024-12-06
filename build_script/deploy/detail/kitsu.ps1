Compare-Object -ReferenceObject (Get-Content -Path "D:\tmp\bin\file_association_http.exe") -DifferenceObject (Get-Content -Path "D:\kitsu\bin\file_association_http.exe")
$Target = "D:\kitsu"
$Tmp = "D:\tmp"
if ((Get-FileHash "$Target\bin\file_association_http.exe").Hash -ne (Get-FileHash "$Tmp\bin\file_association_http.exe").Hash)
{
    &robocopy $Tmp $Target /MIR
}
&robocopy $Tmp $Target /MIR