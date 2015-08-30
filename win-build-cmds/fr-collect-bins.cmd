SET Today=%Date:~10,4%_%Date:~7,2%_%Date:~4,2%
cd bin
mkdir fr
cd fr
mkdir %today%
cd %today%

mkdir rel-mini
mkdir rel

cd ..\..\..\

copy build\fr\lua-client\MinSizeRel\lua_client.exe bin\fr\%today%\rel-mini\
copy build\fr\lua-client\Release\lua_client.exe bin\fr\%today%\rel\


