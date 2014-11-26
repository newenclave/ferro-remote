# Add more folders to ship with the application, here

## TODO: Make it with cmake!!!!

folder_01.source = qml/gui-client
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    frclient.cpp \
    application-data.cpp

include( ferro-remote-templ.pri )

# Installation path
# target.path =

# Please do not modify the following two lines. Required for deployment.
include( qtquick2applicationviewer/qtquick2applicationviewer.pri )
qtcAddDeployment( )

OTHER_FILES += \
    ferro-remote.pri.in

HEADERS += \
    frclient.h \
    application-data.h

