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

  Get-NssmLog
  Get-NssmSer
}

Enter-PSSession -Session $NewSession 



