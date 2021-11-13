cmake_minimum_required(VERSION 3.0)
project(mutebot VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)

IF (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(TD_USE_ASAN)
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)
add_subdirectory(td)

configure_file(config.h.in config.h)

set(CMAKE_C_FLAGS_DEBUG
        "${CMAKE_C_FLAGS_DEBUG} -g3 -O0 -fsanitize=address")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")

add_definitions(-D_POSIX_C_SOURCE=200809L)
set(MUTEBOT_SRC
        main.c
        cmdline.c
	tdutils.c
	handlers.c
	)
add_executable(mutebot ${MUTEBOT_SRC})
target_include_directories(mutebot PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(mutebot PRIVATE tdc)
