mkdir build
cd build
mkdir fr
cd fr
cmake -G "Visual Studio 12" -Wno-dev -DBOOST_LIBRARYDIR="C:/SDK/boost/Boost_1.57/lib"  -DPROTOBUF_INCLUDE_DIR="C:\SDK\ProtoBuf\ProtoBuf_2.4.1\include"  -DPROTOBUF_LIBRARY_DEBUG="C:\SDK\ProtoBuf\ProtoBuf_2.4.1\lib\libprotobuf-vc120-mt-sgd.lib"  -DPROTOBUF_LIBRARY="C:\SDK\ProtoBuf\ProtoBuf_2.4.1\lib\libprotobuf-vc120-mt-s.lib"  -DPROTOBUF_PROTOC_EXECUTABLE="C:\SDK\ProtoBuf\ProtoBuf_2.4.1\bin\protoc.exe" -DBOOST_LIBRARYDIR="C:\SDK\boost\Boost_1.57\lib" -DBOOST_ROOT="C:\SDK\boost\Boost_1.57" -DBOOST_INCLUDEDIR="C:\SDK\boost\Boost_1.57\include" -DLUA_SRC="C:\\SDK\\lua-5.3.0\\src" ../../ferro-remote
cd ..\..

