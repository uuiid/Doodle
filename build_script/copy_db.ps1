param (
  [switch]$Backup,
  [switch]$Copy,
  [string]$Kitsu_Ip = "192.168.40.188"
)

$BackupShare = "\\$Kitsu_Ip\Users\Administrator\AppData\Local\Temp\doodle\backup"
$DataDestination = [System.IO.Path]::GetFullPath("$PSScriptRoot/../build/kitsu_new.db")
$ProjectPath = 'D:/test_db'

# 功能二: 调用 /api/doodle/backup 让服务器备份数据库
function Invoke-ServerBackup {
  Write-Host "使用 Kitsu http://$Kitsu_Ip/api/doodle/backup 进行备份"
  $KitsuCookies = (Get-ItemProperty -Path HKLM:\SOFTWARE\Doodle -Name kitsu_cookies).kitsu_cookies;
  $headers = @{
    "Authorization" = "Bearer $KitsuCookies"
  }
  # 兼容5.1版本 需要加 -UseBasicParsing
  if ($PSVersionTable.PSVersion.Major -lt 6) {
    $res = Invoke-WebRequest -Uri "http://$Kitsu_Ip/api/doodle/backup" -Method Post -Headers $headers -UseBasicParsing
  }
  else {
    $res = Invoke-WebRequest -Uri "http://$Kitsu_Ip/api/doodle/backup" -Method Post -Headers $headers
  }
  # 服务器返回备份文件的完整路径 (json 字符串)
  $l_backup_file = $res.Content
  Write-Host "服务器备份完成: $l_backup_file"
  return $l_backup_file
}

# 功能一: 复制服务器上最新的备份数据库到本地
function Copy-LatestDatabase {
  if (Test-Path $DataDestination) {
    Write-Host "删除旧的数据库: $DataDestination"
    Remove-Item $DataDestination
    if (Test-Path "$DataDestination-shm") {
      Write-Host "删除旧的数据库共享内存文件: $DataDestination-shm"
      Remove-Item "$DataDestination-shm"
    }
    if (Test-Path "$DataDestination-wal") {
      Write-Host "删除旧的数据库日志文件: $DataDestination-wal"
      Remove-Item "$DataDestination-wal"
    }
  }

  # Install-Module PSSQLite
  Import-Module PSSQLite

  $l_list = @(Get-ChildItem -Path $BackupShare -Filter "kitsu_*.db" | Sort-Object LastWriteTime -Descending)
  if ($l_list.Count -eq 0) {
    Write-Host "没有找到备份数据库"
    return;
  }
  $DataSource = $l_list[0].FullName
  Write-Host "使用最新的备份数据库: $DataSource"

  Copy-Item "$DataSource" $DataDestination -Force
  $prjs = Invoke-SqliteQuery -DataSource $DataDestination -Query "select *from project;"
  foreach ($p in $prjs) {
    # //192.168.10.250/dd to D:/
    if ($p.path -eq '') { continue }
    $q = "update project set path = '$ProjectPath' where id=$($p.id);";
    Invoke-SqliteQuery -DataSource $DataDestination -Query $q;
  }

  Invoke-SqliteQuery -DataSource $DataDestination -Query @"
update seedance2_task_2
set status='failed',
    completion_tokens=0
where status == 'preparing';
"@


  $version = Invoke-SqliteQuery -DataSource $DataDestination -Query "pragma user_version;"
  Write-Host "当前数据库版本为 $($version[0].user_version)"
}

if (-not ($Backup -or $Copy)) {
  Write-Host "请指定要执行的功能:"
  Write-Host "  -Backup  调用服务器 /api/doodle/backup 备份数据库"
  Write-Host "  -Copy    复制服务器上最新的备份数据库到本地 $DataDestination"
  Write-Host "示例: .\copy_db.ps1 -Backup ; .\copy_db.ps1 -Copy ; .\copy_db.ps1 -Backup -Copy"
  exit 1
}

if ($Backup) { Invoke-ServerBackup }
if ($Copy) { Copy-LatestDatabase }
