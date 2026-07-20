param (
  [string]$logs
)

$OutputEncoding = [System.Text.Encoding]::UTF8

class update_logs {
  [string]$log;
}

# 如何为空打印错误并返回
if ($logs -ne $null) {
  $KitsuCookies = (Get-ItemProperty -Path HKLM:\SOFTWARE\Doodle -Name kitsu_cookies).kitsu_cookies;
  $headers = @{
    "Authorization" = "Bearer $KitsuCookies"
    "Content-Type"  = "application/json"
  }
  $logs_c = New-Object update_logs
  $logs_c.log = $logs
  $logs_json_s = $logs_c | ConvertTo-Json -Compress
  Invoke-WebRequest -Uri "http://192.168.40.188/api/data/updata-logs" -Method Post -Headers $headers -Body $logs_json_s
}
