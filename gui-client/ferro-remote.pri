
# TODO make it generated

QMAKE_CXXFLAGS += -std=gnu++11

INCLUDEPATH += ../../vtrc/include/vtrc-common/config
INCLUDEPATH += ../../vtrc/include
INCLUDEPATH += ../include

LIBS += /home/data/github/build-ferro-remote/client-core/libferro_remote_client_core.a
LIBS += /home/data/github/build-ferro-remote/protocol/libferro_remote_protocol.a
LIBS += /home/data/github/build-vtrc/vtrc-client/libvtrc_client.a
LIBS += /home/data/github/build-vtrc/vtrc-common/libvtrc_common.a

LIBS += /usr/lib/x86_64-linux-gnu/libboost_system.a          \
        /usr/lib/x86_64-linux-gnu/libboost_thread.a          \
        /usr/lib/x86_64-linux-gnu/libboost_program_options.a \
        /usr/lib/x86_64-linux-gnu/libboost_filesystem.a      \
        /usr/lib/x86_64-linux-gnu/libpthread.so

LIBS += /usr/lib/x86_64-linux-gnu/libprotobuf.so

