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












  Get-NssmLog
  Get-NssmSer
}

Enter-PSSession -Session $NewSession 



