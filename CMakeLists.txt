project(mo-gfx-samples)

set(
	BIN_NAME
	"mo-gfx-examples"
)

include_directories(./include)

file(GLOB HEADERS "include/*.h")
file(GLOB SOURCES "./src/*.cpp")

add_executable(${BIN_NAME} ${SOURCES} ${HEADERS})