
$RootPassword = ConvertTo-SecureString "root" -AsPlainText -Force
$Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList administrator, $RootPassword

(Get-WinEvent -ComputerName 192.168.40.181 -Credential $Credential -FilterHashtable @{
  Logname      = 'Application'
  ProviderName = 'nssm'
  StartTime    = ((Get-Date).AddDays(-1))
})[0..10] |  Format-List -Property TimeCreated, Message
# (Get-Service "doodle_kitsu_*" | Sort-Object Status |  Format-List Name, Status, StartType)
Invoke-Command -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic -ScriptBlock {
    Get-Service "doodle_kitsu_*" | Sort-Object Status |  Format-List Name, Status, StartType
}
Enter-PSSession -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic
