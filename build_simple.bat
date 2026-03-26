@echo off
setlocal EnableDelayedExpansion

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -no_logo

echo [*] Building exMs demo_echo...
echo.

set SRC=runtime/mem/mem.cpp runtime/proc/proc.cpp runtime/fd/fd.cpp runtime/shm/shm.cpp runtime/event/event.cpp core/exabi/src/exabi.cpp core/exloader/src/exloader.cpp core/expdmf/src/expdmf.cpp core/expfc/src/expfc.cpp compat/linux/elf/elf_loader.cpp compat/linux/syscall/dispatcher.cpp

set INC=/Iruntime/mem /Iruntime/proc /Iruntime/fd /Iruntime/shm /Iruntime/event /Icore/exabi/include /Icore/exloader/include /Icore/expdmf/include /Icore/expfc/include /Icompat/linux/elf /Icompat/linux/syscall

cl.exe /EHsc /std:c++17 /Fe:demo_echo.exe /Zi %INC% %SRC% apps/demo_echo/main.cpp 2>&1

if errorlevel 1 (
    echo [!] Build failed
    pause
    exit /b 1
)

echo.
echo [+] Build successful! demo_echo.exe created.

demo_echo.exe

endlocal
