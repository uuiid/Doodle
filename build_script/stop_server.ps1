$KitsuCookies = (Get-ItemProperty -Path HKLM:\SOFTWARE\Doodle -Name kitsu_cookies).kitsu_cookies;
$headers = @{
  "Authorization" = "Bearer $KitsuCookies"
}
$Kitsu_Ip = "192.168.20.89:50025"

Invoke-WebRequest -Uri "http://$Kitsu_Ip/api/doodle/stop-server" -Method Post -Headers $headers -Body "{}" -ContentType "application/json" | Out-Null