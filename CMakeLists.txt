# 设置版本号
cmake_minimum_required(VERSION 3.10)
#设置项目名称
project(opengl-mingw-boilerplate)
#可执行文件生成位置
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# PROJECT_COURCE_DIR表示最外层目录，当前可表示在GAMES101_TO_OPENGL
# file 将所有后缀名为.cpp .h的文件都添加到srcs中
file(GLOB_RECURSE srcs CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/include/*.h)
# 输出参数
message("PROJECT_SOURCE_DIR is ${PROJECT_SOURCE_DIR}")

# 设置路径变量
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")

# 设置静态链接库文件路径
set(GLFW_DIR "${EXT_DIR}/glfw")
set(GLAD_DIR "${EXT_DIR}/glad")
set(GLM_DIR "${EXT_DIR}/glm")

# 设置静态链接库目录
link_directories(${GLFW_DIR}/lib)

# 搜索所有imgui的源文件 
file(GLOB IMGUI_SRC 
        ${PROJECT_SOURCE_DIR}/external/imgui/*.h
        ${PROJECT_SOURCE_DIR}/external/imgui/*.cpp
)

# 可执行文件
add_executable(${PROJECT_NAME} ${srcs} ${IMGUI_SRC})
# 包含头文件目录
target_include_directories(${PROJECT_NAME} 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/external/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/external/Eigen/include/eigen3
)
# 设置语言标准
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# 库
# GLFW
target_link_libraries(${PROJECT_NAME} glfw3)
target_include_directories(${PROJECT_NAME} PUBLIC "${GLFW_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PUBLIC "GLFW_INCLUDE_NONE")

# GLAD
add_library("glad" "${GLAD_DIR}/src/glad.c")
target_include_directories("glad" PUBLIC "${GLAD_DIR}/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${GLAD_DIR}/include")
target_link_libraries(${PROJECT_NAME} "glad" "${CMAKE_DL_LIBS}")

# GLM
target_include_directories(${PROJECT_NAME} PUBLIC "${GLM_DIR}")