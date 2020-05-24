TEMPLATE = app
CONFIG += console c++17 strict_c++ warn_on
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        echoSK.cc \
        main.cpp

HEADERS += \
        echo.hh

unix|win32: LIBS += -lCOSDynamic4

unix|win32: LIBS += -lCOS4

unix|win32: LIBS += -lomniZIOPDynamic4

unix|win32: LIBS += -lomniZIOP4

unix|win32: LIBS += -lomniConnectionMgmt4

unix|win32: LIBS += -lomniCodeSets4

unix|win32: LIBS += -lomniDynamic4

unix|win32: LIBS += -lomniORB4

unix|win32: LIBS += -lomnithread

unix:       LIBS +=-lpthread
