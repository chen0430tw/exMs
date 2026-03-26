# exMs Build Script (PowerShell)
$OutputEncoding = [Console]::OutputEncoding = [Text.UTF8Encoding]::new()

Write-Host "[*] exMs Build Script" -ForegroundColor Cyan
Write-Host ""

# Find MSVC
$msvcBase = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC"
if (Test-Path $msvcBase) {
    $msvcVersion = Get-ChildItem $msvcBase -Directory | Select-Object -First 1 -ExpandProperty Name
    $msvcPath = Join-Path $msvcBase $msvcVersion
} else {
    Write-Host "[!] MSVC not found at $msvcBase" -ForegroundColor Red
    exit 1
}

# Find SDK
$sdkBase = "C:\Program Files (x86)\Windows Kits\10"
$sdkVersion = "10.0.26100.0"
$sdkPath = Join-Path $sdkBase "Include\$sdkVersion"
if (-not (Test-Path $sdkPath)) {
    $sdkVersion = Get-ChildItem (Join-Path $sdkBase "Include") -Directory | Where-Object { $_.Name -match "^10\." } | Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty Name
}

Write-Host "[+] MSVC: $msvcPath" -ForegroundColor Green
Write-Host "[+] SDK: $sdkVersion" -ForegroundColor Green
Write-Host ""

# Set environment
$env:PATH = "$msvcPath\bin\Hostx64\x64;" + $env:PATH
$env:INCLUDE = "$msvcPath\include;$sdkBase\Include\$sdkVersion\ucrt;$sdkBase\Include\$sdkVersion\um;$sdkBase\Include\$sdkVersion\shared"
$env:LIB = "$msvcPath\lib\x64;$sdkBase\Lib\$sdkVersion\ucrt\x64;$sdkBase\Lib\$sdkVersion\um\x64"

# Source files
$srcFiles = @(
    "runtime\mem\mem.cpp",
    "runtime\proc\proc.cpp",
    "runtime\fd\fd.cpp",
    "runtime\shm\shm.cpp",
    "runtime\event\event.cpp",
    "core\exabi\src\exabi.cpp",
    "core\exloader\src\exloader.cpp",
    "core\expdmf\src\expdmf.cpp",
    "core\expfc\src\expfc.cpp",
    "compat\linux\elf\elf_loader.cpp",
    "compat\linux\syscall\dispatcher.cpp",
    "apps\demo_echo\main.cpp"
)

# Include directories
$includes = @(
    "runtime\mem",
    "runtime\proc",
    "runtime\fd",
    "runtime\shm",
    "runtime\event",
    "core\exabi\include",
    "core\exloader\include",
    "core\expdmf\include",
    "core\expfc\include",
    "compat\linux\elf",
    "compat\linux\syscall"
) | ForEach-Object { "/I`"$_`"" }

# Build command
$clArgs = @("/nologo", "/O2", "/MT", "/EHsc", "/std:c++17", "/utf-8", "/Zi") + $includes + $srcFiles + @("/Fe:demo_echo.exe")

Write-Host "[*] Building demo_echo..." -ForegroundColor Yellow
Write-Host ""

# Run cl.exe
$result = & "cl.exe" $clArgs 2>&1
Write-Host $result

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "[+] Build OK: demo_echo.exe" -ForegroundColor Green
    Write-Host ""
    & ".\demo_echo.exe"
} else {
    Write-Host ""
    Write-Host "[!] Build FAILED, exit code: $LASTEXITCODE" -ForegroundColor Red
}
