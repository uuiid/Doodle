$DataSource = "$PSScriptRoot/../build/kitsu_new.db"
Install-Module PSSQLite
Copy-Item "\\192.168.40.181\Dev\kitsu_new.database" $DataSource -Force
$prjs = Invoke-SqliteQuery -DataSource $DataSource -Query "select *from project;"
foreach ($p in $prjs) { 
  # //192.168.10.250/dd to D:/
  if ($p.path -eq '') { continue }  
  $path = 'D:/test_db' + $p.path.Substring(16)
  $q = "update project set path = '$path' where id=$($p.id);";
  Invoke-SqliteQuery -DataSource $DataSource -Query $q;
}