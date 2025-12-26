
# copy dependent files

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

LIBS += -L$$PWD/lib \
        -lRendezvousProto
