# Add more folders to ship with the application, here

## TODO: Make it with cmake!!!!

folder_01.source = qml/gui-client
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

#QMAKE_CFLAGS_RELEASE += -Os -momit-leaf-frame-pointer
#QMAKE_LFLAGS         += -static -static-libgcc
#DEFINES              += QT_STATIC_BUILD

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    fr-client.cpp \
    application-data.cpp \
    fr-client-os.cpp \
    fr-base-component.cpp \
    fr-component-creator.cpp \
    fr-client-fs.cpp \
    fr-client-file.cpp \
    fr-client-fs-iterator.cpp \
    fr-client-component.cpp \
    fr-client-gpio.cpp \
    fr-client-i2c.cpp

include( ferro-remote-templ.pri )

# Installation path
# target.path =

# Please do not modify the following two lines. Required for deployment.
include( qtquick2applicationviewer/qtquick2applicationviewer.pri )
qtcAddDeployment( )

OTHER_FILES += \
    ferro-remote.pri.in

HEADERS += \
    fr-client.h \
    application-data.h \
    fr-client-os.h \
    fr-base-component.h \
    fr-component-creator.h \
    fr-client-fs.h \
    fr-qml-call-wrappers.h \
    fr-client-file.h \
    fr-client-fs-iterator.h \
    fr-client-component.h \
    fr-client-gpio.h \
    fr-client-i2c.h

