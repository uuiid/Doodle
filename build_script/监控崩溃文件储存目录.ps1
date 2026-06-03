<#
.SYNOPSIS
    监控文件夹中的新增文件并发送 Windows 通知（支持等待文件夹创建）
.DESCRIPTION
    若文件夹不存在，脚本会等待其被创建（每2秒检测一次），之后每5分钟扫描一次，
    对比前后文件列表，发现新文件时通过系统弹窗发出通知。
    按 Ctrl+C 可停止脚本。
.PARAMETER FolderPath
    要监控的文件夹路径，默认为当前目录下的 "MonitoredFolder" 文件夹。
.EXAMPLE
    .\Monitor-Folder.ps1 -FolderPath "C:\待处理"
    如果文件夹不存在则等待其出现，然后开始监控。
#>

param(
  [string]$FolderPath = "\\192.168.40.188\Users\Administrator\AppData\Local\Temp\doodle\crashpad_db\reports"
)

# 将相对路径转换为绝对路径（不检查是否存在，仅转换）
$FolderPath = [System.IO.Path]::GetFullPath($FolderPath)
Write-Host "目标文件夹绝对路径: $FolderPath" -ForegroundColor Gray

# 等待文件夹被创建（如果不存在）
while (-not (Test-Path -Path $FolderPath -PathType Container)) {
  Write-Host "文件夹不存在: $FolderPath ，等待创建中... (每2秒检测一次，按 Ctrl+C 退出)" -ForegroundColor Yellow
  Start-Sleep -Seconds 2
}
Write-Host "文件夹已找到: $FolderPath" -ForegroundColor Green

# 初始化：记录现有文件，避免启动时立即通知
Write-Host "开始监控文件夹：$FolderPath (间隔 1 分钟)" -ForegroundColor Green

# 主循环
while ($true) {
  Start-Sleep -Seconds 60

  # 再次确保文件夹存在（防止监控过程中被删除）
  if (-not (Test-Path -Path $FolderPath -PathType Container)) {
    Write-Host "警告：文件夹 $FolderPath 已丢失，等待重新创建..." -ForegroundColor Red
    while (-not (Test-Path -Path $FolderPath -PathType Container)) {
      Start-Sleep -Seconds 2
    }
    Write-Host "文件夹已恢复: $FolderPath" -ForegroundColor Green
    continue
  }
  $newFiles = Get-ChildItem -Path $FolderPath -File -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName

  if ($newFiles.Count -gt 0) {
    $fileNames = $newFiles | ForEach-Object { Split-Path $_ -Leaf }
    $summary = "发现 $($newFiles.Count) 个新文件：`n" + ($fileNames -join "`n")
    Write-Host "[$(Get-Date -Format 'HH:mm:ss')] $summary" -ForegroundColor Cyan
    New-BurntToastNotification -Text "文件夹监控通知", $summary
  }
  else {
    Write-Host "[$(Get-Date -Format 'HH:mm:ss')] 未发现新文件" -ForegroundColor Gray
  }
}