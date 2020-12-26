cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")

message("librtmp->CMakeLists 编译:${PROJECT_SOURCE_DIR}")
add_library(
        rtmp

        STATIC

        amf.c
        hashswf.c
        log.c
        parseurl.c
        rtmp.c)