#-------------------------------------------------
#
# Project created by QtCreator 2011-02-27T16:15:32
#
#-------------------------------------------------

TARGET = evo
TEMPLATE = app

QT       += core gui

#CONFIG   += console
CONFIG += debug_and_release
DEFINES += EVO_QT_SUPPORT

SOURCES += main.cpp \
    Evo.cpp \
	MainWindow.cpp

HEADERS += Evo.h \
	MainWindow.h \
	Plot.h
	
FORMS += MainWindow.ui	

unix {
	INCLUDEPATH += /usr/include/qwt
	INCLUDEPATH += /usr/include/Qt
	LIBS += -L./core/linux
}
win32 {
	INCLUDEPATH += ./qwt/include/
	LIBS += -L./core/win32 -L./qwt
}
	
build_pass:CONFIG(debug, debug|release) {
	win32:LIBS += -lqwtd -lcore_d
	unix: LIBS += -lqwt -lcore_d
}
build_pass:CONFIG(release, debug|release) {
	LIBS += -lqwt -lcore
}

# OpenMP
# C++ flags
QMAKE_CXXFLAGS += -fopenmp
# linker options
QMAKE_LFLAGS += -fopenmp

