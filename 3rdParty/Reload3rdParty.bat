%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Release;Platform="Any CPU" ..\Tools\Build\VS2008\3rdPartyHelper.sln

cd 3rdPartyHelper
3rdPartyHelper.exe ..\3rdPartyDownloads.txt ..\Downloads --cleanup
cd ..

pause