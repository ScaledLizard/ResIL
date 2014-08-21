SOURCES += \
../../src-ILUT/src/ilut_win32.cpp \
../../src-ILUT/src/ilut_opengl.cpp \
../../src-ILUT/src/ilut_allegro.cpp \
../../src-ILUT/src/ilut_states.cpp \
../../src-ILUT/src/ilut_directx.cpp \
../../src-ILUT/src/ilut_main.cpp \
../../src-ILUT/src/ilut_directx9.cpp \
../../src-ILUT/src/ilut_sdlsurface.cpp \
../../src-ILUT/src/ilut_internal.cpp \

HEADERS  += \
../../src-ILUT/include/ilut_opengl.h \
../../src-ILUT/include/ilut_internal.h \
../../src-ILUT/include/ilut_states.h \
../../src-ILUT/include/ilut_allegro.h \
../../include/IL/ilut_config.h \
../../include/IL/ilut.h \

TARGET = resilut
TEMPLATE = lib