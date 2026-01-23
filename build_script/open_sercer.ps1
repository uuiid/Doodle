Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force
$NewSession = New-ServerPSSession
# winget install --id Microsoft.PowerShell
Invoke-Command -Session $NewSession -ScriptBlock {
  function Get-NssmLog {
    $Logs = Get-WinEvent -FilterHashtable @{
      Logname      = 'Application'
      ProviderName = 'nssm'
      StartTime    = ((Get-Date).AddDays(-1))
    } -ErrorAction SilentlyContinue
    if ($Logs) {
      $Logs[0..10] | Select-Object TimeCreated, Id, LevelDisplayName, Message | Format-List  
    }
    else {
      Write-Output "No NSSM logs found in the last 24 hours."
    }
  }

  function Get-NssmSer {
    Get-Service "doodle_kitsu_*" | Sort-Object Status |  Format-List Name, Status, StartType
  }

  function Add-Backup {
    $action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument "-File D:\backup.ps1"
    $trigger = New-ScheduledTaskTrigger -Daily -At "0:01 AM"
    Register-ScheduledTask -Action $action -Trigger $trigger -TaskName "Doodle-Backup" -Description "Doodle 自动备份任务" -RunLevel Highest -Force
  }

  function Add-ClearTmp {
    $action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument "-File D:\clear_tmp.ps1"
    $trigger = New-ScheduledTaskTrigger -Daily -At "0:02 AM"
    Register-ScheduledTask -Action $action -Trigger $trigger -TaskName "Doodle-ClearTmp" -Description "Doodle 自动清理临时文件任务" -RunLevel Highest -Force
  }

  function Get-ClearTmp {
    return Get-ScheduledTask -TaskName "Doodle-ClearTmp"
  }


  function Get-Backup {
    return Get-ScheduledTask -TaskName "Doodle-Backup"
  }


  function Set-NssmReg {
    Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\doodle_kitsu_supplement_2\Parameters\AppExit" -Name "0" -Value "Exit"
    Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\doodle_kitsu_supplement\Parameters\AppExit" -Name "0" -Value "Exit"
  }

  function New-NssmServer {
    $RootPassword = ConvertTo-SecureString "sywh.123" -AsPlainText -Force
    $Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList administrator, $RootPassword

    &D:\nssm.exe install doodle_kitsu_supplement_2 D:/doodle_kitsu_supplement_2/bin/doodle_kitsu_supplement.exe
    &D:\nssm.exe install doodle_kitsu_supplement D:/doodle_kitsu_supplement/bin/doodle_kitsu_supplement.exe

    Set-Service -Name "doodle_kitsu_supplement_2" -Credential $Credential
    Set-Service -Name "doodle_kitsu_supplement" -Credential $Credential

    Set-Service -Name "doodle_kitsu_supplement_2" -StartupType Manual 
    Set-Service -Name "doodle_kitsu_supplement" -StartupType Automatic
  }

  function Get-ComputerInfo {
    $LoadPercentage = Get-WmiObject win32_processor | select -exp LoadPercentage
    $freemem = Get-WmiObject -Class Win32_OperatingSystem

    ""
    "System Name      : {0}" -f $freemem.csname
    "Total Memory (GB): {0}" -f ([math]::round($freemem.TotalVisibleMemorySize / 1mb))
    "Free Memory  (MB): {0}" -f ([math]::round($freemem.FreePhysicalMemory / 1024, 2))
    "CPU Load %       : {0}" -f $LoadPercentage
    ""    
  }

  Get-NssmLog
  Get-NssmSer
}

Enter-PSSession -Session $NewSession 



