
# copy dependent files

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

LIBS += -L$$PWD/lib \
        -lavformat -lavcodec -lswscale -lavutil \
        -lswresample -lavfilter -lx264
