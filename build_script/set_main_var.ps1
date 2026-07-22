$KitsuCookies = (Get-ItemProperty -Path HKLM:\SOFTWARE\Doodle -Name kitsu_cookies).kitsu_cookies;
$DoodleGitRoot = Convert-Path "$PSScriptRoot/../"
$DoodleIp = "192.168.40.188"