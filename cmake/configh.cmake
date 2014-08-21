INCLUDE (CheckIncludeFiles)
# usage: CHECK_INCLUDE_FILES (<header> <RESULT_VARIABLE> )

CHECK_INCLUDE_FILES (malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES ("sys/param.h;sys/mount.h" HAVE_SYS_MOUNT_H)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/../include/IL/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/../include/IL/config.h)

