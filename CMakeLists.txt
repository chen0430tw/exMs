cmake_minimum_required(VERSION 3.16)
project(exMs CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}  # Project root for "runtime/xxx/xxx.hpp" style includes
    # Core modules
    core/exloader/include
    core/exabi/include
    core/expdmf/include
    core/expfc/include
    core/exsvc/include
    # Compat layer
    compat/linux/elf
    compat/linux/syscall
    # Runtime
    runtime/mem
    runtime/proc
    runtime/fd
    runtime/shm
    runtime/event
    runtime/log
)

# Runtime library (static)
add_library(exms_runtime STATIC
    # Runtime modules
    runtime/mem/mem.cpp
    runtime/proc/proc.cpp
    runtime/fd/fd.cpp
    runtime/shm/shm.cpp
    runtime/event/event.cpp
    # Core modules
    core/exabi/src/exabi.cpp
    core/exloader/src/exloader.cpp
    core/expdmf/src/expdmf.cpp
    core/expfc/src/expfc.cpp
    # Compat layer
    compat/linux/elf/elf_loader.cpp
    compat/linux/syscall/dispatcher.cpp
)

# demo_echo application
add_executable(demo_echo
    apps/demo_echo/main.cpp
)

target_link_libraries(demo_echo exms_runtime)

# exboot application
add_executable(exboot_demo
    boot/exboot/main.cpp
)

target_link_libraries(exboot_demo exms_runtime)

# stress_test application
add_executable(stress_test
    apps/stress_test/main.cpp
)

target_link_libraries(stress_test exms_runtime)

# elf_test application
add_executable(elf_test
    apps/elf_test/main.cpp
)

target_link_libraries(elf_test exms_runtime)
