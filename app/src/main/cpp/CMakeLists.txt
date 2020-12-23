cmake_minimum_required(VERSION 3.10.2)
project("player")


message("PROJECT_SOURCE_DIR:${PROJECT_SOURCE_DIR}")
message("CMAKE_ANDROID_ARCH_ABI:${CMAKE_ANDROID_ARCH_ABI}")

set(DIR ${PROJECT_SOURCE_DIR}/../jniLibs/${CMAKE_ANDROID_ARCH_ABI})

message("DIR----->:${DIR}")

#指定h文件的目录，编译过程需要找.h文件
include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB source_file ${PROJECT_SOURCE_DIR}/*)

add_library(
        native-lib
        SHARED
        ${source_file} )


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS -L${DIR}")

#导入FFmpeg的库
#add_library(libavcodec SHARED IMPORTED)
##so文件的位置
#set_target_properties(libavcodec PROPERTIES IMPORTED_LOCATION ${DIR}/libavcodec.so)
#
#add_library(libavfilter SHARED IMPORTED)
#set_target_properties(libavfilter PROPERTIES IMPORTED_LOCATION ${DIR}/libavfilter.so)
#
#add_library(libavformat SHARED IMPORTED)
#set_target_properties(libavformat PROPERTIES IMPORTED_LOCATION ${DIR}/libavformat.so)
#
#add_library(libavutil SHARED IMPORTED)
#set_target_properties(libavutil PROPERTIES IMPORTED_LOCATION ${DIR}/libavutil.so)
#
#add_library(libswresample SHARED IMPORTED)
#set_target_properties(libswresample PROPERTIES IMPORTED_LOCATION ${DIR}/libswresample.so)
#
#add_library(libswscale SHARED IMPORTED)
#set_target_properties(libswscale PROPERTIES IMPORTED_LOCATION ${DIR}/libswscale.so)


find_library(
        log-lib
        log)

target_link_libraries(
        native-lib
        ${log-lib}
        avfilter
        avformat
        swresample
        swscale
        avutil
        avcodec
        z
        android)

