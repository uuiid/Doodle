$DataDestination = "$PSScriptRoot/../build/kitsu_new.db"
# Install-Module PSSQLite
Import-Module PSSQLite

$l_list = Get-ChildItem -Path "\\192.168.0.181\Dev\kitsu_data\backup" -Filter "kitsu_*.db" | Sort-Object LastWriteTime -Descending
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
  $path = 'D:/test_db' + $p.path.Substring(16)
  $q = "update project set path = '$path' where id=$($p.id);";
  Invoke-SqliteQuery -DataSource $DataDestination -Query $q;
}