cmake_minimum_required(VERSION 3.16)
project(exMsSkeleton CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    core/exloader/include
    core/exabi/include
    core/expdmf/include
    core/expfc/include
    core/exsvc/include
    compat/linux/elf
    compat/linux/syscall
)

add_executable(exboot_demo
    boot/exboot/main.cpp
)

add_executable(demo_echo
    apps/demo_echo/main.cpp
    compat/linux/elf/elf_loader.cpp
    compat/linux/syscall/dispatcher.cpp
    core/expdmf/src/expdmf.cpp
    core/expfc/src/expfc.cpp
)
