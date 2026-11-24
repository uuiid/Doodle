# dsh-tool-everything 端到端验证：临时 profile 挂载插件 -> headless 真实调用 -> 护栏 -> 清理
#
#   ./test/e2e.ps1
#
# 完全不触碰任何现有 profile；临时 profile 与 temporary session 用完即删。

[CmdletBinding()]
param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot),
    [string]$Scope = 'E:\Doodle\src',
    [string]$Endpoint = 'http://127.0.0.1:8099/',
    [string]$ExpectedNeedle = 'retarget_rotations',
    [string]$ExpectedFile = 'motion_postprocess.cpp'
)

$ErrorActionPreference = 'Stop'
$profileName = 'ese2e' + (Get-Random -Minimum 1000 -Maximum 9999)
$profileDir = Join-Path $HOME ".dsh\profiles\$profileName"
$overlay = Join-Path $PSScriptRoot 'llm-overlay.yml'
$failures = 0

function Step($text) { Write-Host "`n=== $text ===" -ForegroundColor Cyan }
function Pass($text) { Write-Host "ok   $text" -ForegroundColor Green }
function Fail($text) { Write-Host "FAIL $text" -ForegroundColor Red; $script:failures++ }

try {
    Step "前置：Everything HTTP 可用？"
    try {
        $probe = Invoke-WebRequest -Uri "$Endpoint`?json=1&count=1&search=" -UseBasicParsing -TimeoutSec 10
        if ($probe.StatusCode -eq 200) { Pass "HTTP $Endpoint 存活" } else { Fail "HTTP 状态 $($probe.StatusCode)" }
    } catch { Fail "Everything HTTP 不可用：$($_.Exception.Message)"; throw }

    Step "建临时 profile $profileName（headless 模板）"
    & dsh $profileName --from-default-profile headless --dump-config | Out-Null
    if ($LASTEXITCODE -ne 0) { Fail "profile 初始化失败"; throw }
    Pass "已创建 $profileDir"

    Step "安装插件（file: 依赖）"
    & dsh plugin --profile $profileName add "file:$Root" 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { Fail "插件安装被拒绝"; throw }
    Pass "插件已安装并加入 bundles"

    Step "E2E-1：真实内容检索"
    $prompt1 = "Call the esearch tool exactly once: content search for the text $ExpectedNeedle limited to path $Scope, maxResults 10. Then list the absolute paths returned, one per line, and nothing else."
    $out1 = (& dsh $profileName --patch $overlay $prompt1 2>&1 | Out-String)
    if ($out1 -match [regex]::Escape($ExpectedFile)) { Pass "检索命中 $ExpectedFile" } else { Fail "检索输出里没有 $ExpectedFile`n$out1" }

    Step "E2E-2：护栏（盘根必须被拒）"
    $prompt2 = "Call the esearch tool exactly once with pattern $ExpectedNeedle, content true, and path E:\. Then report verbatim the tool's returned text."
    $out2 = (& dsh $profileName --patch $overlay $prompt2 2>&1 | Out-String)
    if ($out2 -match 'filesystem root') { Pass "盘根查询被拒绝" } else { Fail "盘根没有被拒绝`n$out2" }

    Step "E2E-3：护栏（无范围必须被拒）"
    $patchFile = Join-Path $profileDir 'cordis.patch.yml'
    @"
- id: tool-everything
  name: dsh-tool-everything
  config:
    scopes: []
"@ | Set-Content $patchFile -Encoding utf8
    $prompt3 = "Call the esearch tool exactly once with pattern $ExpectedNeedle and content true, and DO NOT pass a path argument. Report verbatim the tool's returned text."
    Push-Location $HOME
    try { $out3 = (& dsh $profileName --patch $overlay $prompt3 2>&1 | Out-String) } finally { Pop-Location }
    if ($out3 -match 'no path scope') { Pass "无范围查询被拒绝" } else { Fail "无范围查询没有被拒绝`n$out3" }
} finally {
    Step "清理临时 profile"
    if (Test-Path $profileDir) {
        Remove-Item $profileDir -Recurse -Force -ErrorAction SilentlyContinue
        if (Test-Path $profileDir) { Fail "临时 profile 删除失败：$profileDir" } else { Pass "已删除 $profileDir" }
    }
}

Write-Host ""
if ($failures -eq 0) { Write-Host "ALL PASS" -ForegroundColor Green; exit 0 }
Write-Host "$failures FAILURE(S)" -ForegroundColor Red; exit 1
