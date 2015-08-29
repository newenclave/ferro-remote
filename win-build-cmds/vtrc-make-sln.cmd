mkdir build
cd build
mkdir vtrc
cd vtrc
cmake -G "Visual Studio 12" -DBOOST_LIBRARYDIR="C:/SDK/boost/Boost_1.57/lib"  -DPROTOBUF_INCLUDE_DIR="C:\SDK\ProtoBuf\ProtoBuf_2.4.1\include"  -DPROTOBUF_LIBRARY_DEBUG="C:\SDK\ProtoBuf\ProtoBuf_2.4.1\lib\libprotobuf-vc120-mt-sgd.lib"  -DPROTOBUF_LIBRARY="C:\SDK\ProtoBuf\ProtoBuf_2.4.1\lib\libprotobuf-vc120-mt-s.lib"  -DPROTOBUF_PROTOC_EXECUTABLE="C:\SDK\ProtoBuf\ProtoBuf_2.4.1\bin\protoc.exe" -DBOOST_LIBRARYDIR="C:\SDK\boost\Boost_1.57\lib" -DBOOST_ROOT="C:\SDK\boost\Boost_1.57" -DBOOST_INCLUDEDIR="C:\SDK\boost\Boost_1.53\include" -DCMAKE_BUILD_TYPE=Release ../../vtrc
cd ..\..
