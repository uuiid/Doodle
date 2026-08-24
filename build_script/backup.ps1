$Kitsu_Ip = "127.0.0.1"
Write-Host "使用 Kitsu http://$Kitsu_Ip/api/doodle/backup 进行备份"
$KitsuCookies = (Get-ItemProperty -Path HKLM:\SOFTWARE\Doodle -Name kitsu_cookies).kitsu_cookies;
$headers = @{
  "Authorization" = "Bearer $KitsuCookies"
}

$res = Invoke-WebRequest -Uri "http://$Kitsu_Ip/api/doodle/backup" -Method Post -Headers $headers 

Copy-Item -Path $res.Content -Destination "\\192.168.0.67\bakk3\doodlebak\kitsu_backup" -Force

# 归档前一日 token 消耗统计 (seedance2_tokens_person_date POST)
$Yesterday = (Get-Date).AddDays(-1).ToString("yyyy-MM-dd")
Write-Host "归档 $Yesterday 的 Token 消耗统计 http://$Kitsu_Ip/api/seedance2/tokens/person/date/$Yesterday"
Invoke-WebRequest -Uri "http://$Kitsu_Ip/api/seedance2/tokens/person/date/$Yesterday" -Method Post -Headers $headers -ContentType "application/json" -Body "{}" | Out-Null

# 重置所有人员当周剩余 token 额度 (seedance2_tokens POST, 不传 remaining_tokens 时重置为最大值), 仅在周日执行
if ((Get-Date).DayOfWeek -eq 'Sunday') {
  Write-Host "重置当周 Token 额度 http://$Kitsu_Ip/api/seedance2/tokens"
  Invoke-WebRequest -Uri "http://$Kitsu_Ip/api/seedance2/tokens" -Method Post -Headers $headers -ContentType "application/json" -Body "{}" | Out-Null
}