$Doodle_Root = "E:/Doodle"
$Kitsu_Root = "E:/source/Kitsu"
# 设置环境变量, 将git输出改为utf-8编码
[System.Environment]::SetEnvironmentVariable("LANG", "en_US.UTF-8", "Process")
# powershell 编解码设置为utf-8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

function Get-GitMsg {
  param (
    [string] $GitRoot
  )
  # 从git仓库中获取提交消息 (上周一周的)
  Set-Location $GitRoot
  # 计算上周的日期范围, 今天不一定是周一, 所以先找到上周一的日期
  $today = Get-Date
  $daysToLastMonday = ($today.DayOfWeek.value__ + 6) % 7 + 7
  $lastMonday = $today.AddDays(-$daysToLastMonday)
  $lastSunday = $lastMonday.AddDays(6)
  $since = $lastMonday.ToString("yyyy-MM-dd")
  $until = $lastSunday.ToString("yyyy-MM-dd")
  # 获取git提交消息, utf-8编码
  $gitLog = git log --since="$since" --until="$until" --pretty=format:"%h - %s (%an, %ad)" --date=short | Out-String
  # 循环排除 ... 提交
  # 循环去除 git 中的消息id (b4c5b71b0 - )
  # 循环 将行尾括号内的日期以外的内容去除 (uuiid, 2026-01-19) -> 2026-01-19
  # 并在日期后面添加 ➕ 日期 ✅ 结束日期
  # 最后去除行尾多余空格, 换行符等
  # 最后去除多余的空行
  $gitLog = $gitLog -split "`n" | Where-Object { $_ -notmatch "\.\.\. " } |  ForEach-Object { $_ -replace "^[a-f0-9]{7,} - ", "" } | 
  ForEach-Object { $_ -replace "\s\(.*?,\s([0-9]{4}-[0-9]{2}-[0-9]{2})\)", " ➕ `$1 ✅ $until" } |
  ForEach-Object { $_.TrimEnd() } |  
  Where-Object { $_ -ne "" } | 
  Out-String
  return $gitLog  
}

# 获取Doodle和Kitsu的git提交消息
$Doodle_GitMsg = Get-GitMsg -GitRoot $Doodle_Root
$Kitsu_GitMsg = Get-GitMsg -GitRoot $Kitsu_Root

Write-Host $Doodle_GitMsg
Write-Host "-----------------------------------"
Write-Host $Kitsu_GitMsg

# 生成周报内容并写入文件
Set-Content -Path "../../周报.md" -Value $Doodle_GitMsg, "`n-----------------------------------`n", $Kitsu_GitMsg -Encoding UTF8