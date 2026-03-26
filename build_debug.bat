@echo off
setlocal EnableDelayedExpansion

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -no_logo

echo Starting build... > build_log.txt
date /t >> build_log.txt
time /t >> build_log.txt

set INC=/Iruntime/mem /Iruntime/proc /Iruntime/fd /Iruntime/shm /Iruntime/event /Icore/exabi/include /Icore/exloader/include /Icore/expdmf/include /Icore/expfc/include /Icompat/linux/elf /Icompat/linux/syscall

cl.exe /EHsc /std:c++17 /Fe:demo_echo.exe %INC% ^
    runtime/mem/mem.cpp ^
    runtime/proc/proc.cpp ^
    runtime/fd/fd.cpp ^
    runtime/shm/shm.cpp ^
    runtime/event/event.cpp ^
    core/exabi/src/exabi.cpp ^
    core/exloader/src/exloader.cpp ^
    core/expdmf/src/expdmf.cpp ^
    core/expfc/src/expfc.cpp ^
    compat/linux/elf/elf_loader.cpp ^
    compat/linux/syscall/dispatcher.cpp ^
    apps/demo_echo/main.cpp >> build_log.txt 2>&1

echo. >> build_log.txt
echo Return code: %errorlevel% >> build_log.txt

type build_log.txt

endlocal
