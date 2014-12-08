
# TODO make it generated

QMAKE_CXXFLAGS += -std=gnu++11

INCLUDEPATH +=  \
	C:/github/vtrc/include \
	C:/github/vtrc/include/vtrc-common/config \
	C:/github/vtrc/include/vtrc-common/protocol \
	C:/SDK/ProtoBuf/protobuf-2.6.1/src \
	C:/SDK/boost/boost_1_57_0 \
	C:/SDK/ProtoBuf/protobuf-2.6.1/src \
	C:/github/ferro-remote/include

LIBS +=  \
	C:/github/fr-b/client-core/libferro_remote_client_core.a \
	C:/github/fr-b/protocol/libferro_remote_protocol.a \
	C:/github/vtrc-b/vtrc-client/libvtrc_client.a \
	C:/github/vtrc-b/vtrc-common/libvtrc_common.a \
	C:/SDK/boost/boost_1_57_0/stage/lib/libboost_system-mgw49-mt-s-1_57.a \
	C:/SDK/boost/boost_1_57_0/stage/lib/libboost_thread-mgw49-mt-s-1_57.a \
	C:/SDK/boost/boost_1_57_0/stage/lib/libboost_program_options-mgw49-mt-s-1_57.a \
	C:/SDK/boost/boost_1_57_0/stage/lib/libboost_filesystem-mgw49-mt-s-1_57.a \
        C:/SDK/ProtoBuf/protobuf-2.6.1/src/.libs/libprotobuf.a \
        C:/SDK/Qt/Tools/mingw491_32/i686-w64-mingw32/lib/libws2_32.a \
        C:/SDK/Qt/Tools/mingw491_32/lib/gcc/i686-w64-mingw32/4.9.1/libstdc++.a
