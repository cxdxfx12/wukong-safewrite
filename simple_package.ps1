$ErrorActionPreference = "Stop"
$projectDir = "C:\Users\ccc\Desktop\wukong-main"
Set-Location $projectDir

Write-Host "=== 悟空快递 - 打包 ===" -ForegroundColor Cyan

# 检查编译
if (Test-Path "build\Release\WukongFreight.exe") {
    Write-Host "OK: 编译文件存在" -ForegroundColor Green
} else {
    Write-Host "ERROR: 请先编译" -ForegroundColor Red
    exit 1
}

# 创建包目录
if (Test-Path "package") { Remove-Item -Recurse -Force "package" }
mkdir "package" | Out-Null

# 复制文件
Copy-Item "build\Release\WukongFreight.exe" "package\"
Write-Host "OK: 主程序" -ForegroundColor Green

# 创建 README
@"
# 悟空快递运费计算系统 v1.0.0
日期：2026-07-09
开发：杭州喵喵至家网络有限公司

双击运行 WukongFreight.exe
"@ | Out-File "package\README.txt" -Encoding UTF8

Write-Host "OK: 文档" -ForegroundColor Green
Write-Host ""
Write-Host "=== 打包完成 ===" -ForegroundColor Green
Write-Host "目录：$projectDir\package" -ForegroundColor Yellow
