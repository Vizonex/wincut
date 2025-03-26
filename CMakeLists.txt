cmake_minimum_required(VERSION 3.7.2)
project("wincut" VERSION 0.0.1)
set("PROJECT_DESCRIPTION" "A Duplicut fork for windows")

# Setup Sources list...
set(
    wincut_sources

    vendor/termiWin.c
    src/bytesize.c
    src/chunk.c
    src/dconfig.c
    src/dedupe.c
    src/error.c
    src/file.c
    src/getopt.c
    src/hmap.c
    src/line.c
    src/main.c
    src/meminfo.c
    src/memstate.c
    src/mkstemp.c
    src/mman.c
    src/optparse.c
    src/status.c
    src/thpool.c
    src/uinput.c
)

set(
    wincut_include_directories
    "include"
    vendor/pthreads
    vendor
)

# mimalloc optimizations isnt ready yet but toying around with the executable is...
set(wincut_libraries psapi shell32 user32 advapi32 bcrypt libpthreadVC3)

add_executable(
    wincut ${wincut_sources}
)
# target_compile_definitions(wincut)
target_link_directories(wincut PUBLIC "lib")
target_link_libraries(wincut PUBLIC ${wincut_libraries})
target_include_directories(wincut PUBLIC ${wincut_include_directories})
