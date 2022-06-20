CMAKE_MINIMUM_REQUIRED(VERSION 3.8.0)


PROJECT(HDR)
SET(TARGET "HDR")

add_compile_definitions(UNICODE _UNICODE)

IF(MSVC)
# 采用 Windows内部的窗口 来为了以后opengl显示窗口
# set(CMAKE_EXE_LINKER_FLAGS"${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")

# 添加 被 msvc 警告危险接口
add_definitions(-D_CRT_SECURE_NO_WARNINGS) 

    IF (WIN7_SYSTEM_SUPPORT MATCHES ON)
	    add_compile_definitions(_WIN32_WINNT=0x601)
    ENDIF()

    # 设置MSVC的启动项，否则Cmake生成的工程默认启动的工程永远是ALL_BUILD
    IF (NOT ${CMAKE_VERSION} VERSION_LESS "3.6.0")
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    ENDIF ()

ENDIF(MSVC)

# 通过宏命令传递shader路径给主程序
add_definitions(-DDIR_PATH="${CMAKE_CURRENT_SOURCE_DIR}/")
# add_definitions(-DSHADER_PATH="${CMAKE_CURRENT_SOURCE_DIR}/shader/")


SET(INSTALL_PATH "${PROJECT_SOURCE_DIR}/bin")

LINK_DIRECTORIES("${PROJECT_SOURCE_DIR}/bin") #指定链接路径
SET(CAMKE_CXX_FLAGS "{CAMKE_CXX_FLAGS} -lglew32")

# add_definitions("-I<path to directx headers>")
INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}")
# INCLUDE_DIRECTORIES("C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/um")


FILE(GLOB SRC_LIST "./*.cpp")
# file(GLOB  SRC_LIST "./protoc/*.cc")
SOURCE_GROUP("source" FILES ${SRC_LIST})

FILE(GLOB  HEAD_LIST "./*.h")
SOURCE_GROUP("include" FILES ${HEAD_LIST})

FILE(GLOB  COMMON_LIST "./common/*.cpp" "./common/*.h")
SOURCE_GROUP("common" FILES ${COMMON_LIST})

FILE(GLOB  SHADER_LIST "./shader/*.*")
SOURCE_GROUP("shader" FILES ${SHADER_LIST})


SET(ALL_SOURCES
${SRC_LIST}
${HEAD_LIST}
${COMMON_LIST}
${SHADER_LIST}
)


SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${INSTALL_PATH})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  ${INSTALL_PATH})


# ADD_LIBRARY(${TARGET} ${ALL_SOURCES})
ADD_EXECUTABLE(${TARGET} ${ALL_SOURCES}) #只有在 add_executable() 中的文件才会被 source_group 分组


# GlU32.Lib 固定管线
TARGET_LINK_LIBRARIES(${TARGET} "${PROJECT_SOURCE_DIR}/third_party/glew32.lib")
TARGET_LINK_LIBRARIES(${TARGET} "opengl32.lib")
# TARGET_LINK_LIBRARIES(${TARGET} "${PROJECT_SOURCE_DIR}/protoc/libprotobufd.lib")


# add vscode launch.json for debug
IF (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # using Clang
ELSEIF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # using GCC
  configure_file("${PROJECT_SOURCE_DIR}/launch_gdb.json" "${PROJECT_SOURCE_DIR}/.vscode/launch.json" COPYONLY)
ELSEIF (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # using Visual Studio C++
  configure_file("${PROJECT_SOURCE_DIR}/launch_msvc.json" "${PROJECT_SOURCE_DIR}/.vscode/launch.json" COPYONLY)
ENDIF()
