set(sources
    main.c
    diskio.c
)

add_executable(mm.prg ${sources})
target_link_libraries(mm.prg PRIVATE mega65libc)
