QT += core

TARGET = QCVimgLib
TEMPLATE = lib

CONFIG += c++17
CONFIG -= debug_and_release

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QCVIMGLIB_LIBRARY

HEADERS += \
    qcvimg.h \
    qcvimglib_decl.h \



SOURCES += \
    qcvimg.cpp \

    
win32: {
    include("c:/dev/opencv/opencv.pri")
}

unix: !macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4
}

unix: macx {
    INCLUDEPATH += "/usr/local/include"
    LIBS += -L"/usr/local/lib" -lopencv_world
}
