$Kitsu_Ip = "127.0.0.1"
Write-Host "使用 Kitsu http://$Kitsu_Ip/api/doodle/backup 进行备份"
$KitsuCookies = (Get-ItemProperty -Path HKLM:\SOFTWARE\Doodle -Name kitsu_cookies).kitsu_cookies;
$headers = @{
  "Authorization" = "Bearer $KitsuCookies"
}

$res = Invoke-WebRequest -Uri "http://$Kitsu_Ip/api/doodle/backup" -Method Post -Headers $headers 

Copy-Item -Path $res.Content -Destination "\\192.168.0.67\bakk3\doodlebak\kitsu_backup" -Force