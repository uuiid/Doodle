Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force
$NewSession = New-ServerPSSession
# (Get-Service "doodle_kitsu_*" | Sort-Object Status |  Format-List Name, Status, StartType)
Invoke-Command -Session $NewSession -ScriptBlock {
  function Get-NssmLog {
    (Get-WinEvent -FilterHashtable @{
      Logname      = 'Application'
      ProviderName = 'nssm'
      StartTime    = ((Get-Date).AddDays(-1))
    })[0..10] |  Format-List -Property TimeCreated, Message
  }

  function Get-NssmSer {
    Get-Service "doodle_kitsu_*" | Sort-Object Status |  Format-List Name, Status, StartType
  }

  function Add-Backup {
    $action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument "-File D:\backup.ps1"
    $trigger = New-ScheduledTaskTrigger -Daily -At "9:00 AM"
    Register-ScheduledTask -Action $action -Trigger $trigger -TaskName "Doodle-Backup" -Description "Doodle 自动备份任务" -RunLevel Highest -Force
  }


  function Get-Backup {
    return Get-ScheduledTask -TaskName "Doodle-Backup"
  }


  function Set-NssmReg {
    Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\doodle_kitsu_supplement_2\Parameters\AppExit" -Name "0" -Value "Exit"
    Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\doodle_kitsu_supplement\Parameters\AppExit" -Name "0" -Value "Exit"
  }

  function New-NssmServer {
    $RootPassword = ConvertTo-SecureString "root" -AsPlainText -Force
    $Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList administrator, $RootPassword

    &D:\nssm.exe install doodle_kitsu_supplement_2 D:/doodle_kitsu_supplement_2/bin/doodle_kitsu_supplement.exe
    &D:\nssm.exe install doodle_kitsu_supplement D:/doodle_kitsu_supplement/bin/doodle_kitsu_supplement.exe

    Set-Service -Name "doodle_kitsu_supplement_2" -Credential $Credential
    Set-Service -Name "doodle_kitsu_supplement" -Credential $Credential

    Set-Service -Name "doodle_kitsu_supplement_2" -StartupType Manual 
    Set-Service -Name "doodle_kitsu_supplement" -StartupType Automatic
  }







  Get-NssmLog
  Get-NssmSer
}

Enter-PSSession -Session $NewSession 



