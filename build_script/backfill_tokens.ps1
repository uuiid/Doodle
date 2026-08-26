$Kitsu_Ip = "192.168.40.188"
$KitsuCookies = (Get-ItemProperty -Path HKLM:\SOFTWARE\Doodle -Name kitsu_cookies).kitsu_cookies;
$headers = @{
  "Authorization" = "Bearer $KitsuCookies"
}

# 从今天到3天前, 逐日归档 token 消耗统计, 修补历史缺失
$End = (Get-Date).Date
$Start = $End.AddDays(-3)
for ($Date = $Start; $Date -le $End; $Date = $Date.AddDays(1)) {
  $DateStr = $Date.ToString("yyyy-MM-dd")
  Write-Host "归档 $DateStr 的 Token 消耗统计 http://$Kitsu_Ip/api/seedance2/tokens/person/date/$DateStr"
  try {
    Invoke-WebRequest -Uri "http://$Kitsu_Ip/api/seedance2/tokens/person/date/$DateStr" -Method Post -Headers $headers -ContentType "application/json" -Body "{}"  | Out-Null
  }
  catch {
    Write-Warning "归档 $DateStr 失败: $($_.Exception.Message)"
  }
}
