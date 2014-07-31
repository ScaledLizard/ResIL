SOURCES += \
../../src-ILU/src/ilu_scale.cpp \
../../src-ILU/src/ilu_scale3d.cpp \
../../src-ILU/src/ilu_rotate.cpp \
../../src-ILU/src/ilu_states.cpp \
../../src-ILU/src/ilu_alloc.cpp \
../../src-ILU/src/ilu_manip.cpp \
../../src-ILU/src/ilu_mipmap.cpp \
../../src-ILU/src/ilu_region.cpp \
../../src-ILU/src/ilu_noise.cpp \
../../src-ILU/src/ilu_filter_rcg.cpp \
../../src-ILU/src/ilu_scale2d.cpp \
../../src-ILU/src/ilu_internal.cpp \
../../src-ILU/src/ilu_scaling.cpp \
../../src-ILU/src/ilu_utilities.cpp \
../../src-ILU/src/ilu_main.cpp \
../../src-ILU/src/ilu_filter.cpp \
../../src-ILU/src/ilu_error.cpp \

HEADERS  += \
../.../../src-ILU/include/ilu_internal.h \
../.../../src-ILU/include/ilu_region.h \
../.../../src-ILU/include/ilu_states.h \
../.../../src-ILU/include/ilu_error/ilu_err-german.h \
../.../../src-ILU/include/ilu_error/ilu_err-arabic.h \
../.../../src-ILU/include/ilu_error/ilu_err-english.h \
../.../../src-ILU/include/ilu_error/ilu_err-japanese.h \
../.../../src-ILU/include/ilu_error/ilu_err-spanish.h \
../.../../src-ILU/include/ilu_error/ilu_err-dutch.h \
../.../../src-ILU/include/ilu_error/ilu_err-french.h \
../.../../src-ILU/include/ilu_filter.h \
../.../../src-ILU/include/ilu_alloc.h \
../../include/IL/ilu_region.h \
../../include/IL/ilu.h \

TARGET = resilu
TEMPLATE = lib