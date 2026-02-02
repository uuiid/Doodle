$Doodle_Root = "E:/Doodle"
$Kitsu_Root = "E:/source/Kitsu"
# 设置环境变量, 将git输出改为utf-8编码
[System.Environment]::SetEnvironmentVariable("LANG", "en_US.UTF-8", "Process")
# powershell 编解码设置为utf-8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Import-Module ImportExcel

function Get-GitMsg {
  param (
    [string] $GitRoot,
    [string] $Date
  )
  # 从git仓库中获取提交消息 传入时间当天的
  Set-Location $GitRoot
 
  # 获取当天git提交消息, utf-8编码
  $gitLog = git log --since="$Date 00:00:00" --until="$Date 23:59:59"  --pretty=format:"%s" --date=short --encoding=UTF-8 | Out-String
  # 循环排除 ... 提交
  # 循环去除 git 中的消息id (b4c5b71b0 - )
  # 循环 将行尾括号内的日期以外的内容去除 (uuiid, 2026-01-19) -> 2026-01-19
  # 并在日期后面添加 ➕ 日期 ✅ 结束日期
  # 最后去除行尾多余空格, 换行符等
  # 最后去除多余的空行
  $gitLog = $gitLog -split "`n" |
  ForEach-Object { $_.TrimEnd() } |  
  Where-Object { $_ -notmatch "\.\.\." } |   
  Where-Object { $_ -ne "" } | 
  Out-String
  return $gitLog  
}

# Excel 格式内容 "项目名称|版本|姓名|开始日期|结束日期|工作量(天)|备注|工作内容"

class ExcelRow {
  [string] $项目名称
  [string] $版本
  [string] $姓名
  [string] $开始日期
  [string] $结束日期
  [int] $工作量
  [string] $备注
  [string] $工作内容
}
# $excelData = @(); 
# return;

# 迭代上个月的每一天
$Today = Get-Date
$LastMonth = $Today.AddMonths(-1)
$DaysInLastMonth = [DateTime]::DaysInMonth($LastMonth.Year, $LastMonth.Month)
$excelData = for ($day = 1; $day -le $DaysInLastMonth; $day++) {
  $date = Get-Date -Year $LastMonth.Year -Month $LastMonth.Month -Day $day
  $dateString = $date.ToString("yyyy-MM-dd")
  # Write-Host "Processing date: $dateString"

  # 获取Doodle和Kitsu的git提交消息
  $Doodle_GitMsg = Get-GitMsg -GitRoot $Doodle_Root -Date $dateString
  # $Kitsu_GitMsg = Get-GitMsg -GitRoot $Kitsu_Root -Date $dateString

  if ($Doodle_GitMsg -ne "") {
    # Write-Host "Doodle commits on $dateString :`n$Doodle_GitMsg`n"
  
    # 生成Excel格式内容
    [ExcelRow]@{
      项目名称 = "doodle"
      版本   = "v3.6.0"
      姓名   = "姓名"
      开始日期 = $dateString
      结束日期 = $dateString
      工作量  = 1
      备注   = ""
      # $Doodle_GitMsg 转换为单行字符串
      工作内容 = $Doodle_GitMsg -replace "`n", " "
    }
  }
}

$excelData | Export-Excel -Path "E:/月报生成示例.xlsx" -WorksheetName "月报" -AutoSize -ClearSheet

# Write-Host $Doodle_GitMsg
# Write-Host "-----------------------------------"
# Write-Host $Kitsu_GitMsg

# # 生成周报内容并写入文件
# Set-Content -Path "../../周报.md" -Value $Doodle_GitMsg, "`n-----------------------------------`n", $Kitsu_GitMsg -Encoding UTF8