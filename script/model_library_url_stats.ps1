#Requires -Version 7.0
<#
.SYNOPSIS
    统计 model_library 路由的访问次数与访问时间.

.DESCRIPTION
    从 doodle_lib 日志 (格式 "[时间] [socket_N] [info] 请求 url METHOD PATH" /
    "回复 url METHOD PATH") 中提取 src/doodle_lib/http_method/model_library/model_library.h:11-46
    声明的 6 条路由的访问记录, 按方法+路由模板聚合次数, 并输出每次访问的时间明细与
    N 分钟时间桶分布.

    "访问" 以 "请求 url" 行为准计数, "回复 url" 行只用于计算耗时与成功率, 不重复计数.

.PARAMETER LogDir
    日志目录.

.PARAMETER OutputDir
    结果输出目录; 生成 <时间戳>_model_library_url_stats.csv / .json / .txt.

.PARAMETER BucketMinutes
    时间桶宽度(分钟), 用于时间分布统计.

.PARAMETER Since
    只统计此时间之后的记录.

.PARAMETER Until
    只统计此时间之前的记录.

.PARAMETER NoReportFile
    只打印到输出, 不写任何结果文件. 用于输出目录不可写时.

.EXAMPLE
    ./model_library_url_stats.ps1

.EXAMPLE
    ./model_library_url_stats.ps1 -Since '2026-10-15 00:00' -BucketMinutes 60

.EXAMPLE
    ./model_library_url_stats.ps1 -NoReportFile
#>
[CmdletBinding()]
param(
  [string] $LogDir = 'E:\Doodle\build\log2',
  [string] $OutputDir = 'E:\Doodle\build',
  [int] $BucketMinutes = 60,
  [datetime] $Since = [datetime]::MinValue,
  [datetime] $Until = [datetime]::MaxValue,
  [switch] $NoReportFile
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

if ($BucketMinutes -lt 1) { throw "BucketMinutes 必须 >= 1, 当前为 $BucketMinutes" }
if (-not (Test-Path -LiteralPath $LogDir)) { throw "日志目录不存在: $LogDir" }
if (-not (Test-Path -LiteralPath $OutputDir)) { New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null }

$routeMethods = [ordered]@{
  '/api/doodle/model_library/context'                = @('GET')
  '/api/doodle/model_library/assets'                 = @('GET', 'POST')
  '/api/doodle/model_library/assets/{id_}'           = @('PUT', 'DELETE')
  '/api/doodle/model_library/assets_tree'            = @('GET', 'POST', 'PATCH')
  '/api/doodle/model_library/assets_tree/{id_}'      = @('PUT', 'DELETE')
}

$uuidPattern = '/[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$'
$uuidRx      = [regex]::new($uuidPattern)

function Resolve-RouteTemplate {
  param([string] $Path)
  if ($uuidRx.IsMatch($Path)) { return $uuidRx.Replace($Path, '/{id_}') }
  return $Path
}

$logFileRx = [regex]::new('^doodle_lib\.\d+\.\d+\.txt$')
$lineRx    = [regex]::new(
  '^\[(?<time>\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d{3})\] \[(?<thread>[^\]]+)\] \[(?<level>\w+)\] (?<kind>\S+) url (?<method>\S+) (?<path>/[^ ]*)'
)

$logFiles = @(
  Get-ChildItem -LiteralPath $LogDir -File |
    Where-Object { $logFileRx.IsMatch($_.Name) } |
    Sort-Object @{ Expression = { [int]($_.Name -split '\.')[2] } }
)
if ($logFiles.Count -eq 0) { throw "在 $LogDir 中没有找到 doodle_lib.*.txt 日志文件" }

Write-Host "扫描 $($logFiles.Count) 个日志文件: $LogDir" -ForegroundColor Cyan

$rawLines = 0
$modelLines = 0
$requestEvents = New-Object System.Collections.Generic.List[object]
$pendingByThread = @{}

foreach ($file in $logFiles) {
  $reader = [System.IO.File]::OpenText($file.FullName)
  try {
    while ($null -ne ($line = $reader.ReadLine())) {
      $rawLines++
      if ($line.IndexOf('model_library', [System.StringComparison]::Ordinal) -lt 0) { continue }

      $m = $lineRx.Match($line)
      if (-not $m.Success) { continue }

      $kind = $m.Groups['kind'].Value
      if ($kind -ne '请求' -and $kind -ne '回复') { continue }

      $path = $m.Groups['path'].Value
      if (-not $path.StartsWith('/api/doodle/model_library/', [System.StringComparison]::Ordinal)) { continue }

      $modelLines++
      $stamp = [datetime]::ParseExact($m.Groups['time'].Value, 'yyyy-MM-dd HH:mm:ss.fff', [cultureinfo]::InvariantCulture)
      if ($stamp -lt $Since -or $stamp -gt $Until) { continue }

      $event = [pscustomobject]@{
        Time      = $stamp
        Thread    = $m.Groups['thread'].Value
        Method    = $m.Groups['method'].Value
        Path      = $path
        Template  = Resolve-RouteTemplate -Path $path
        File      = $file.Name
        DurationMs = $null
        Replied   = $false
      }

      if ($kind -eq '请求') {
        $requestEvents.Add($event)
        if (-not $pendingByThread.ContainsKey($event.Thread)) {
          $pendingByThread[$event.Thread] = New-Object System.Collections.Generic.Queue[object]
        }
        $pendingByThread[$event.Thread].Enqueue($event)
        continue
      }

      if (-not $pendingByThread.ContainsKey($event.Thread)) { continue }
      $threadQueue = $pendingByThread[$event.Thread]
      if ($threadQueue.Count -eq 0) { continue }
      $reply = $threadQueue.Dequeue()
      $reply.Replied = $true
      $reply.DurationMs = [math]::Round(($event.Time - $reply.Time).TotalMilliseconds, 3)
    }
  } finally {
    $reader.Dispose()
  }
}

$summary = New-Object System.Collections.Generic.List[object]
foreach ($template in $routeMethods.Keys) {
  foreach ($method in $routeMethods[$template]) {
    $hits = @($requestEvents | Where-Object { $_.Template -eq $template -and $_.Method -eq $method })
    $durations = @($hits | Where-Object { $null -ne $_.DurationMs } | ForEach-Object { $_.DurationMs })
    $stat = $null
    if ($durations.Count -gt 0) {
      $stat = $durations | Measure-Object -Minimum -Maximum -Average -Sum
    }
    $summary.Add([pscustomobject]@{
      Method       = $method
      Route        = $template.Replace('{id_}', '{id}')
      Count        = $hits.Count
      Replied      = $durations.Count
      First        = if ($hits.Count -gt 0) { ($hits | Measure-Object Time -Minimum).Minimum } else { $null }
      Last         = if ($hits.Count -gt 0) { ($hits | Measure-Object Time -Maximum).Maximum } else { $null }
      AvgMs        = if ($null -ne $stat) { [math]::Round($stat.Average, 3) } else { $null }
      MinMs        = if ($null -ne $stat) { $stat.Minimum } else { $null }
      MaxMs        = if ($null -ne $stat) { $stat.Maximum } else { $null }
      TotalMs      = if ($null -ne $stat) { [math]::Round($stat.Sum, 3) } else { $null }
    })
  }
}

$accessLog = @($requestEvents | Sort-Object Time, Method, Path)

$buckets = @(
  $requestEvents |
    Group-Object { $_.Time.AddTicks(-($_.Time.Ticks % ($BucketMinutes * [TimeSpan]::TicksPerMinute))).Ticks } |
    Sort-Object { [long]$_.Name } |
    ForEach-Object {
      $start = [datetime][long]$_.Name
      $end   = $start.AddMinutes($BucketMinutes)
      $group = @($_.Group)
      [pscustomobject]@{
        WindowStart = $start
        WindowEnd   = $end
        Count       = $group.Count
        Context     = @($group | Where-Object Template -eq '/api/doodle/model_library/context').Count
        Assets      = @($group | Where-Object Template -eq '/api/doodle/model_library/assets').Count
        AssetsId    = @($group | Where-Object Template -eq '/api/doodle/model_library/assets/{id_}').Count
        Tree        = @($group | Where-Object Template -eq '/api/doodle/model_library/assets_tree').Count
        TreeId      = @($group | Where-Object Template -eq '/api/doodle/model_library/assets_tree/{id_}').Count
      }
    }
)

$grandTotal   = $requestEvents.Count
$activeRoutes = @($summary | Where-Object Count -gt 0)
$timeRange    = if ($grandTotal -gt 0) {
  "{0:yyyy-MM-dd HH:mm:ss.fff} .. {1:yyyy-MM-dd HH:mm:ss.fff}" -f ($accessLog | Select-Object -First 1).Time, ($accessLog | Select-Object -Last 1).Time
} else { '(无访问记录)' }

$stamp  = Get-Date -Format 'yyyyMMdd_HHmmss'
$base   = Join-Path $OutputDir "${stamp}_model_library_url_stats"
$csvOut = "$base.csv"
$jsonOut = "$base.json"
$txtOut = "$base.txt"

$report = New-Object System.Collections.Generic.List[string]
$report.Add('model_library 路由访问统计')
$report.Add("日志目录    : $LogDir")
$report.Add("日志文件数  : $($logFiles.Count)")
$report.Add("扫描行数    : $rawLines")
$report.Add("匹配行数    : $modelLines")
$report.Add("时间范围    : $timeRange")
$report.Add("总访问次数  : $grandTotal")
$report.Add("时间桶宽度  : $BucketMinutes 分钟")
$report.Add('')
$report.Add('按 方法+路由 统计:')
$report.Add(($summary | Format-Table -AutoSize | Out-String).TrimEnd())
$report.Add('')
$report.Add('时间分布:')
$report.Add(($buckets | Format-Table -AutoSize | Out-String).TrimEnd())
$report.Add('')
$report.Add('访问明细(时间 / 线程 / 方法 / 路径 / 耗时ms):')
$report.Add(($accessLog | Format-Table Time, Thread, Method, Path, DurationMs -AutoSize | Out-String).TrimEnd())

if (-not $NoReportFile) {
  $summary  | Export-Csv -LiteralPath $csvOut -NoTypeInformation -Encoding utf8
  $accessLog | Export-Csv -LiteralPath "${base}_access.csv" -NoTypeInformation -Encoding utf8
  $buckets  | Export-Csv -LiteralPath "${base}_buckets.csv" -NoTypeInformation -Encoding utf8
  [pscustomobject]@{
    LogDir        = $LogDir
    LogFiles      = $logFiles.Count
    ScannedLines  = $rawLines
    ModelLines    = $modelLines
    Since         = $Since
    Until         = $Until
    BucketMinutes = $BucketMinutes
    TotalAccess   = $grandTotal
    TimeRange     = $timeRange
    Summary       = $summary
    Buckets       = $buckets
    AccessLog     = $accessLog
  } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $jsonOut -Encoding utf8
  Set-Content -LiteralPath $txtOut -Value $report -Encoding utf8
}

Write-Host ''
Write-Host '================ model_library 路由访问统计 ================' -ForegroundColor Cyan
Write-Host ("日志目录   : {0}  ({1} 个文件, 扫描 {2} 行, 匹配 {3} 行)" -f $LogDir, $logFiles.Count, $rawLines, $modelLines)
Write-Host ("时间范围   : {0}" -f $timeRange)
Write-Host ("总访问次数 : {0}  (有访问的路由: {1}/{2})" -f $grandTotal, $activeRoutes.Count, $summary.Count)
Write-Host ''
Write-Host '--- 按 方法+路由 统计 ---' -ForegroundColor Yellow
$summary | Format-Table Method, Route, Count, Replied, First, Last, AvgMs, MinMs, MaxMs -AutoSize | Out-String | Write-Host
Write-Host ("--- 时间分布 ({0} 分钟一桶) ---" -f $BucketMinutes) -ForegroundColor Yellow
if ($buckets.Count -gt 0) {
  $buckets | Format-Table WindowStart, WindowEnd, Count, Context, Assets, AssetsId, Tree, TreeId -AutoSize | Out-String | Write-Host
} else {
  Write-Host '(无访问记录)'
}
Write-Host '--- 访问明细 ---' -ForegroundColor Yellow
if ($accessLog.Count -gt 0) {
  $accessLog | Format-Table Time, Thread, Method, Path, DurationMs -AutoSize | Out-String | Write-Host
} else {
  Write-Host '(无访问记录)'
}
Write-Host '--- 结果文件 ---' -ForegroundColor Green
if ($NoReportFile) {
  Write-Host '(已指定 -NoReportFile, 未写结果文件)'
} else {
  Write-Host "统计汇总 : $csvOut"
  Write-Host "访问明细 : ${base}_access.csv"
  Write-Host "时间分布 : ${base}_buckets.csv"
  Write-Host "JSON     : $jsonOut"
  Write-Host "文本报告 : $txtOut"
}
