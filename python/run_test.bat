pushd %~dp0

cd D:\perforce\eve-frontier\stream
set BUILDFLAVOR=trinitydev
python D:\perforce\eve-frontier\stream\eve\updateBinaries.py client
cd client
D:\perforce\eve-frontier\stream\bin\x64\ExeFileConsole.exe @D:\perforce\eve-frontier\stream\client\libDefault.args @D:\perforce\eve-frontier\stream\client\resDefault.args /buildflavor=%BUILDFLAVOR% /py %~dp0/controller_test.py

popd