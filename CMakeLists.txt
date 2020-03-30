cmake_minimum_required(VERSION 3.1)
project(mo-gfx-samples)

set(
	BIN_NAME
	"mo-gfx-examples"
)

# Set C++11
set (CMAKE_CXX_STANDARD 11)

# Set Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG $<1:${CMAKE_SOURCE_DIR}/bin>)

include_directories(
    include/
    thirdparty/glfw/include/
	thirdparty/mo-gfx/include/
	thirdparty/glm/
	thirdparty/spdlog/include/
)

# Spdlog Building
add_subdirectory(thirdparty/spdlog)

# GLFW Building
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
add_subdirectory(thirdparty/glfw)

# GLM Building
add_subdirectory(thirdparty/glm)

file(GLOB HEADERS "include/*.h")
file(GLOB SOURCES "src/*.cpp")

add_executable(${BIN_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${BIN_NAME} spdlog glfw ${GLFW_LIBRARIES} glm mo-gfx)