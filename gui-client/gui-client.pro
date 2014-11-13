# Add more folders to ship with the application, here

## TODO: Make it with cmake!!!!

folder_01.source = qml/gui-client
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

QMAKE_CXXFLAGS += -std=gnu++11

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp

INCLUDEPATH += ../../vtrc/include/vtrc-common/config
INCLUDEPATH += ../../vtrc/include
INCLUDEPATH += ../include

LIBS += /home/data/github/build-ferro-remote/client-core/libferro_remote_client_core.a
LIBS += /home/data/github/build-ferro-remote/protocol/libferro_remote_protocol.a
LIBS += /home/data/github/build-vtrc/vtrc-client/libvtrc_client.a
LIBS += /home/data/github/build-vtrc/vtrc-common/libvtrc_common.a

LIBS += /usr/lib/x86_64-linux-gnu/libboost_system.a \
        /usr/lib/x86_64-linux-gnu/libboost_thread.a \
        /usr/lib/x86_64-linux-gnu/libboost_program_options.a \
        /usr/lib/x86_64-linux-gnu/libboost_filesystem.a \
        /usr/lib/x86_64-linux-gnu/libpthread.so

LIBS += /usr/lib/x86_64-linux-gnu/libprotobuf.so

# Installation path
# target.path =

# Please do not modify the following two lines. Required for deployment.
include( qtquick2applicationviewer/qtquick2applicationviewer.pri )
qtcAddDeployment( )

