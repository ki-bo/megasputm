cmake_minimum_required(VERSION 3.5)
set(LLVM_MOS_PLATFORM mega65)
find_package(llvm-mos-sdk REQUIRED)
project(helloworld VERSION 1.0.0 LANGUAGES C)

add_subdirectory(mega65-libc)

add_executable(hello.prg main.c)
target_link_libraries(hello.prg PRIVATE mega65libc)
