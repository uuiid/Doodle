#Requires -Version 7.0
<#
.SYNOPSIS
    迭代删除 SQLite 数据库中所有违反外键约束的行，直到 PRAGMA foreign_key_check 返回空。

.DESCRIPTION
    SQLite 没有「一条语句删除全部外键违规行」的语法；而且删掉一行可能让引用它的子行
    变成新的孤儿，所以必须迭代到不动点（fixpoint）。本脚本每一轮：

      1. 执行 `PRAGMA foreign_key_check;`，把违规清单（表名|rowid|父表|fkid）导出到文件；
      2. 按表聚合 rowid，生成 `DELETE FROM "表" WHERE rowid IN (SELECT r FROM _fk_bad WHERE t='表');`
      3. 在单个事务里（PRAGMA foreign_keys=OFF）执行删除；
      4. 重复，直到没有任何违规。

    ── 两个必须知道的坑 ──────────────────────────────────────────────
    (1) 必须使用 `PRAGMA foreign_key_check;` 语句形式。
        如果库里存在一张真实的 `pragma_foreign_key_check` 表（本库就有），它会遮蔽
        SQLite 的同名虚拟表，使 `SELECT * FROM pragma_foreign_key_check` 永远返回
        0 行（假阴性）。脚本会自动检测并提示。

    (2) 带 FTS5 全文索引的表（本库是 `entity`）有维护索引的触发器。如果索引使用了
        自定义分词器（本库是 `jieba`，只由 Doodle 进程注册），独立的 sqlite3.exe
        执行 DELETE 时会报 `no such tokenizer: jieba`。脚本会把这类触发器临时
        DROP 掉，清理完再原样恢复（DDL 从 sqlite_master 读取）。
        代价：被删掉的 entity 行在 `entity_fts` 索引里会留下悬挂条目，
        需要由注册了 jieba 的进程（Doodle 本体）执行 `rebuild` 才能修复。
        脚本结束时会报告悬挂条目数量。

.PARAMETER Database
    目标 SQLite 数据库文件路径。

.PARAMETER SqliteExe
    sqlite3.exe 路径。默认 <仓库>/build/sqlite3.exe。

.PARAMETER DryRun
    只检查并输出违规统计，不删除、不备份。

.PARAMETER NoBackup
    跳过备份（默认先复制一份 <db>.bak_yyyyMMdd_HHmmss）。

.PARAMETER MaxRounds
    最大迭代轮数，默认 50。达到上限仍未清空则报错退出。

.PARAMETER SkipTables
    永不删除的表名列表。这些表里的违规只统计、不处理（用于把 entity 这类
    带 FTS 索引的表留到应用内处理）。

.EXAMPLE
    .\fix_foreign_key_violations.ps1 ..\build\kitsu_new.db -DryRun

.EXAMPLE
    .\fix_foreign_key_violations.ps1 ..\build\kitsu_new.db

.EXAMPLE
    .\fix_foreign_key_violations.ps1 ..\build\kitsu_new.db -SkipTables entity
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Database,

    [string]$SqliteExe,

    [switch]$DryRun,

    [switch]$NoBackup,

    [int]$MaxRounds = 50,

    [string[]]$SkipTables = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ================================================================ 纯 PowerShell 辅助
# 注意：所有 sqlite3.exe 调用都放在脚本顶层。在「函数返回值被赋值」的调用链里
# 启动原生进程会被沙箱的管道限制拒绝。
function ConvertTo-SqlitePath([string]$Path) { $Path -replace '\\', '/' }

function New-QueryScript {
    param([string]$Path, [string]$Sql, [string]$OutputFile)
    @"
.mode list
.headers off
.output $(ConvertTo-SqlitePath $OutputFile)
$Sql
.output stdout
"@ | Set-Content -LiteralPath $Path -Encoding utf8
}

# 从 foreign_key_check 输出文件聚合违规行 -> @{ 表名 = HashSet[long] }
function Get-ViolationMap {
    param([string]$RawFile)
    $map = @{}
    if (-not (Test-Path -LiteralPath $RawFile)) { return $map }
    foreach ($line in (Get-Content -LiteralPath $RawFile)) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        $parts = $line.Split('|')
        if ($parts.Count -lt 2) { continue }
        $tbl = $parts[0]
        $rid = $parts[1]
        if ([string]::IsNullOrWhiteSpace($rid)) {
            throw "表 '$tbl' 是 WITHOUT ROWID 表，无法用 rowid 定位违规行，请手工处理。"
        }
        if (-not $map.ContainsKey($tbl)) {
            $map[$tbl] = [System.Collections.Generic.HashSet[long]]::new()
        }
        [void]$map[$tbl].Add([long]$rid)
    }
    return $map
}

# 生成「一轮删除」的 SQL
function New-DeleteScript {
    param([hashtable]$Map, [string]$Path, [string[]]$Skip)
    $sb = [System.Text.StringBuilder]::new()
    [void]$sb.AppendLine('PRAGMA foreign_keys=OFF;')
    [void]$sb.AppendLine('BEGIN IMMEDIATE;')
    [void]$sb.AppendLine('CREATE TEMP TABLE _fk_bad(t TEXT NOT NULL, r INTEGER NOT NULL);')

    $tables = @($Map.Keys | Where-Object { $Skip -notcontains $_ } | Sort-Object)
    $chunk = [System.Collections.Generic.List[string]]::new()
    foreach ($tbl in $tables) {
        $esc = $tbl.Replace("'", "''")
        foreach ($rid in $Map[$tbl]) {
            $chunk.Add("('$esc',$rid)")
            if ($chunk.Count -ge 500) {
                [void]$sb.AppendLine('INSERT INTO _fk_bad(t,r) VALUES ' + ($chunk -join ',') + ';')
                $chunk.Clear()
            }
        }
    }
    if ($chunk.Count -gt 0) {
        [void]$sb.AppendLine('INSERT INTO _fk_bad(t,r) VALUES ' + ($chunk -join ',') + ';')
        $chunk.Clear()
    }
    foreach ($tbl in $tables) {
        $quoted = '"' + $tbl.Replace('"', '""') + '"'
        $esc = $tbl.Replace("'", "''")
        [void]$sb.AppendLine("DELETE FROM $quoted WHERE rowid IN (SELECT r FROM _fk_bad WHERE t='$esc');")
    }
    [void]$sb.AppendLine('DROP TABLE _fk_bad;')
    [void]$sb.AppendLine('COMMIT;')
    Set-Content -LiteralPath $Path -Value $sb.ToString() -Encoding utf8
}

# ================================================================ 路径准备
if (-not $SqliteExe) { $SqliteExe = Join-Path $PSScriptRoot '..\build\sqlite3.exe' }
if (-not (Test-Path -LiteralPath $SqliteExe)) { throw "找不到 sqlite3.exe: $SqliteExe" }
if (-not (Test-Path -LiteralPath $Database)) { throw "找不到数据库文件: $Database" }

$SqliteExe = (Resolve-Path -LiteralPath $SqliteExe).Path
$db = (Resolve-Path -LiteralPath $Database).Path

# 工作目录与数据库同级且不含空格：sqlite3 的 .read 对含空格路径不可靠
$work = Join-Path (Split-Path -Parent $db) '_fkfix_tmp'
if (Test-Path -LiteralPath $work) { Remove-Item -LiteralPath $work -Recurse -Force }
New-Item -ItemType Directory -Path $work | Out-Null

$checkSql  = Join-Path $work 'check.sql'
$deleteSql = Join-Path $work 'delete.sql'
$rawOut    = Join-Path $work 'violations.txt'
$qSql      = Join-Path $work 'q.sql'
$qOut      = Join-Path $work 'q.txt'

Write-Host "数据库 : $db"
Write-Host "sqlite3: $SqliteExe"
if ($SkipTables.Count -gt 0) { Write-Host "跳过表 : $($SkipTables -join ', ')" }

New-QueryScript -Path $checkSql -Sql 'PRAGMA foreign_key_check;' -OutputFile $rawOut

# -bail: 任何语句出错就立即退出，避免 sqlite3 默认「继续执行到 COMMIT」造成部分提交
function Invoke-Sql { param([string]$Path) & $SqliteExe -bail $db ".read $(ConvertTo-SqlitePath $Path)" }

# ================================================================ 环境自检
New-QueryScript -Path $qSql -OutputFile $qOut -Sql @'
SELECT 'shadow=' || count(*) FROM sqlite_master WHERE type='table' AND name='pragma_foreign_key_check';
SELECT 'ftsdll=' || name || '|' || sql FROM sqlite_master WHERE type='table' AND sql LIKE 'CREATE VIRTUAL TABLE%';
SELECT 'ftstrig=' || tr.name || '|' || tr.tbl_name FROM sqlite_master tr, sqlite_master vt
WHERE tr.type='trigger' AND vt.type='table' AND vt.sql LIKE 'CREATE VIRTUAL TABLE%'
  AND instr(tr.sql, vt.name) > 0;
'@
Invoke-Sql $qSql
$probe = @(Get-Content -LiteralPath $qOut | Where-Object { $_ })

$shadow = $false
$ftsContent = @{}   # fts 表名 -> content 表名
$ftsTriggers = @()  # 触发器名
foreach ($line in $probe) {
    if ($line -like 'shadow=*') {
        $shadow = ([int]($line.Substring(7))) -gt 0
    }
    elseif ($line -like 'ftsdll=*') {
        $rest = $line.Substring(7)
        $i = $rest.IndexOf('|')
        if ($i -lt 0) { continue }
        $name = $rest.Substring(0, $i)
        $ddl = $rest.Substring($i + 1)
        $m = [regex]::Match($ddl, "content\s*=\s*'([^']+)'")
        $ftsContent[$name] = if ($m.Success) { $m.Groups[1].Value } else { '' }
    }
    elseif ($line -like 'ftstrig=*') {
        $rest = $line.Substring(8)
        $i = $rest.IndexOf('|')
        $ftsTriggers += if ($i -ge 0) { $rest.Substring(0, $i) } else { $rest }
    }
}

if ($shadow) {
    Write-Warning @'
库中存在真实表 pragma_foreign_key_check，它会遮蔽 SQLite 的同名虚拟表。
因此 `SELECT * FROM pragma_foreign_key_check` 会永远返回 0 行（假阴性），
必须使用 `PRAGMA foreign_key_check;` 语句形式。本脚本不受影响。
'@
}

# ================================================================ DryRun
if ($DryRun) {
    Invoke-Sql $checkSql
    $v = Get-ViolationMap -RawFile $rawOut
    if ($v.Count -eq 0) { Write-Host "`n没有外键违规。" -ForegroundColor Green }
    else {
        $total = ($v.Values | ForEach-Object { $_.Count } | Measure-Object -Sum).Sum
        Write-Host "`n发现 $total 行外键违规，分布在 $($v.Count) 张表：" -ForegroundColor Yellow
        $v.GetEnumerator() | Sort-Object { $_.Value.Count } -Descending |
            ForEach-Object { Write-Host ("  {0,-45} {1,7}" -f $_.Key, $_.Value.Count) }
    }
    Remove-Item -LiteralPath $work -Recurse -Force
    return
}

# ================================================================ 备份
if (-not $NoBackup) {
    $bak = "$db.bak_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
    Write-Host "备份 -> $bak"
    Copy-Item -LiteralPath $db -Destination $bak -Force
}

# ================================================================ 处理 FTS 触发器
$restoreSql = Join-Path $work 'restore_triggers.sql'
$droppedTriggers = @()
if ($ftsTriggers.Count -gt 0) {
    $droppedTriggers = $ftsTriggers
    $inList = ($ftsTriggers | ForEach-Object { "'" + $_.Replace("'", "''") + "'" }) -join ','
    New-QueryScript -Path $qSql -OutputFile $restoreSql -Sql "SELECT sql || ';' FROM sqlite_master WHERE type='trigger' AND name IN ($inList);"
    Invoke-Sql $qSql

    $dropSql = Join-Path $work 'drop_triggers.sql'
    New-QueryScript -Path $dropSql -OutputFile (Join-Path $work 'drop.txt') -Sql (
        ($ftsTriggers | ForEach-Object { "DROP TRIGGER IF EXISTS `"$_`";" }) -join "`n"
    )
    Invoke-Sql $dropSql

    Write-Warning @"
检测到 FTS5 索引触发器，已临时摘除：
  $($ftsTriggers -join ', ')
原因：这些触发器需要自定义分词器，独立 sqlite3.exe 没有注册它。
清理结束后会自动原样恢复。但被删掉的 entity 行会在 entity_fts 里留下悬挂条目，
需要由 Doodle 本体（注册了 jieba）执行 rebuild 才能修复。
"@
}

# ================================================================ 迭代删除
$reportPath = "$db.fk_violations.tsv"
$report = [System.Collections.Generic.List[string]]::new()
$report.Add("# 原始违规明细: 表名<TAB>rowid<TAB>父表<TAB>fkid")

$round = 0
$grandTotal = 0
$skippedViolations = 0

while ($true) {
    $round++
    if ($round -gt $MaxRounds) { throw "已迭代 $MaxRounds 轮仍未清空外键违规，已中止。" }

    Invoke-Sql $checkSql
    $v = Get-ViolationMap -RawFile $rawOut
    if ($v.Count -eq 0) { break }

    if ($round -eq 1) {
        foreach ($line in (Get-Content -LiteralPath $rawOut)) {
            if (-not [string]::IsNullOrWhiteSpace($line)) { $report.Add(($line -replace '\|', "`t")) }
        }
    }

    $doomed = @($v.Keys | Where-Object { $SkipTables -notcontains $_ })
    $skipNow = @($v.Keys | Where-Object { $SkipTables -contains $_ })
    foreach ($t in $skipNow) { $skippedViolations += $v[$t].Count }

    if ($doomed.Count -eq 0) {
        Write-Host "`n只剩被跳过的表的违规，停止。" -ForegroundColor Yellow
        break
    }

    $rowsThisRound = ($doomed | ForEach-Object { $v[$_].Count } | Measure-Object -Sum).Sum
    $grandTotal += $rowsThisRound
    Write-Host "`n[第 $round 轮] 待删除 $rowsThisRound 行，涉及 $($doomed.Count) 张表"
    foreach ($tbl in ($doomed | Sort-Object)) { Write-Host ("    - {0,-45} {1,7}" -f $tbl, $v[$tbl].Count) }
    foreach ($tbl in ($skipNow | Sort-Object)) { Write-Host ("    ~ {0,-45} {1,7}  (跳过)" -f $tbl, $v[$tbl].Count) }

    New-DeleteScript -Map $v -Path $deleteSql -Skip $SkipTables
    Invoke-Sql $deleteSql
}

# ================================================================ 恢复触发器
if ($droppedTriggers.Count -gt 0) {
    Invoke-Sql $restoreSql
    Write-Host "`n已恢复 FTS 触发器。"
}

# ================================================================ 收尾与最终校验
Set-Content -LiteralPath $reportPath -Value $report -Encoding utf8

if ($grandTotal -eq 0) { Write-Host "`n没有外键违规，数据库无需修改。" -ForegroundColor Green }
else {
    Write-Host "`n完成：共 $round 轮，删除 $grandTotal 行。" -ForegroundColor Green
    Write-Host "原始违规明细: $reportPath"
}

Invoke-Sql $checkSql
$final = @(Get-Content -LiteralPath $rawOut | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
Write-Host "最终 PRAGMA foreign_key_check 剩余违规: $($final.Count)"

# FTS 索引一致性（悬挂条目会导致 MATCH 报 "fts5: missing row ... from content table"）
foreach ($fts in $ftsContent.Keys) {
    $content = $ftsContent[$fts]
    if ([string]::IsNullOrWhiteSpace($content)) { continue }
    $check = "SELECT '$fts dangling=' || count(*) FROM ${fts}_docsize d WHERE NOT EXISTS (SELECT 1 FROM `"$content`" e WHERE e.rowid = d.id);"
    New-QueryScript -Path $qSql -OutputFile $qOut -Sql $check
    Invoke-Sql $qSql
    $line = (Get-Content -LiteralPath $qOut -Raw).Trim()
    if ($line) {
        $n = [int]($line -replace '.*dangling=', '')
        $color = if ($n -gt 0) { 'Yellow' } else { 'Green' }
        Write-Host "$fts 索引悬挂条目: $n" -ForegroundColor $color
        if ($n -gt 0) {
            Write-Host "  -> 需由 Doodle 进程执行: INSERT INTO $fts($fts) VALUES('rebuild');" -ForegroundColor Yellow
        }
    }
}

Remove-Item -LiteralPath $work -Recurse -Force
