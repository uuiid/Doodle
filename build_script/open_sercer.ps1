Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force
$NewSession = New-ServerPSSession
# (Get-Service "doodle_kitsu_*" | Sort-Object Status |  Format-List Name, Status, StartType)
Invoke-Command -Session $NewSession -ScriptBlock {
  Get-Service "doodle_kitsu_*" | Sort-Object Status |  Format-List Name, Status, StartType
  function Get-NssmLog {
    (Get-WinEvent -FilterHashtable @{
      Logname      = 'Application'
      ProviderName = 'nssm'
      StartTime    = ((Get-Date).AddDays(-1))
    })[0..10] |  Format-List -Property TimeCreated, Message
  }
  Get-NssmLog
}

Connect-PSSession -Session $NewSession



