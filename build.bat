@echo off
setlocal

echo [*] exMs Build Script
echo.

set MSVC=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207
set SDK=C:\Program Files (x86)\Windows Kits\10

rem Try to find MSVC version if default doesn't exist
if not exist "%MSVC%" (
    for /d %%i in ("C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\*.*") do (
        set MSVC=%%i
        goto :found_msvc
    )
    :found_msvc
)

echo [+] MSVC: %MSVC%

rem Find SDK version
if not exist "%SDK%\Include\10.0.26100.0" (
    for /d %%i in ("%SDK%\Include\10.0.*") do (
        set SDKVER=%%~nxi
        goto :found_sdk
    )
) else (
    set SDKVER=10.0.26100.0
)
:found_sdk

echo [+] SDK: %SDKVER%

set PATH=%MSVC%\bin\Hostx64\x64;%PATH%
set INCLUDE=%MSVC%\include;%SDK%\Include\%SDKVER%\ucrt;%SDK%\Include\%SDKVER%\um;%SDK%\Include\%SDKVER%\shared
set LIB=%MSVC%\lib\x64;%SDK%\Lib\%SDKVER%\ucrt\x64;%SDK%\Lib\%SDKVER%\um\x64

cd /d "%~dp0"

echo.
echo [*] Building demo_echo...
echo.

cl.exe /nologo /O2 /MT /EHsc /std:c++17 /utf-8 /Zi ^
    /Iruntime\mem /Iruntime\proc /Iruntime\fd /Iruntime\shm /Iruntime\event ^
    /Icore\exabi\include /Icore\exloader\include /Icore\expdmf\include /Icore\expfc\include ^
    /Icompat\linux\elf /Icompat\linux\syscall ^
    runtime\mem\mem.cpp ^
    runtime\proc\proc.cpp ^
    runtime\fd\fd.cpp ^
    runtime\shm\shm.cpp ^
    runtime\event\event.cpp ^
    core\exabi\src\exabi.cpp ^
    core\exloader\src\exloader.cpp ^
    core\expdmf\src\expdmf.cpp ^
    core\expfc\src\expfc.cpp ^
    compat\linux\elf\elf_loader.cpp ^
    compat\linux\syscall\dispatcher.cpp ^
    apps\demo_echo\main.cpp ^
    /Fe:demo_echo.exe

if %ERRORLEVEL% == 0 (
    echo.
    echo [+] Build OK: %~dp0demo_echo.exe
    echo.
    demo_echo.exe
) else (
    echo [!] Build FAILED
)

endlocal
