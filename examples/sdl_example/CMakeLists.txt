find_package(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})
add_executable(sdltest
	sdl_test.c
)
target_link_libraries(sdltest
	ILUT
	IL
	${SDL_LIBRARY}
)
