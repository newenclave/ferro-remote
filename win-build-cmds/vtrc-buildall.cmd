cd build\vtrc
"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" /t:build /nologo /p:Configuration="Release" /p:Platform="Win32" /consoleloggerparameters:PerformanceSummary /m /v:n "vtrc.sln"
"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" /t:build /nologo /p:Configuration="MinSizeRel" /p:Platform="Win32" /consoleloggerparameters:PerformanceSummary /m /v:n "vtrc.sln"
"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" /t:build /nologo /p:Configuration="Debug" /p:Platform="Win32" /consoleloggerparameters:PerformanceSummary /m /v:n "vtrc.sln"
"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" /t:build /nologo /p:Configuration="RelWithDebInfo" /p:Platform="Win32" /consoleloggerparameters:PerformanceSummary /m /v:n "vtrc.sln"

cd ..\..