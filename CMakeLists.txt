project(mo-gfx-examples)

set(
	BIN_NAME
	"examples"
)

include_directories(./include)
file(GLOB SOURCES ./src/*.cpp)

add_executable(${BIN_NAME} ${SOURCES} ${HEADERS})