# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
FILE(GLOB_RECURSE u8g2_sources
	${CMAKE_SOURCE_DIR}/.pio/libdeps/seeed_xiao_esp32c6/U8g2/src/*.c
	${CMAKE_SOURCE_DIR}/.pio/libdeps/seeed_xiao_esp32c6/U8g2/src/*.cpp
)

idf_component_register(
	SRCS ${app_sources} ${u8g2_sources}
	INCLUDE_DIRS
		.
		${CMAKE_SOURCE_DIR}/.pio/libdeps/seeed_xiao_esp32c6/U8g2/src
)
