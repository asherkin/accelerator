IF EXIST breakpad\NUL GOTO HASBREAKPAD
mkdir breakpad
:HASBREAKPAD
cd breakpad

IF EXIST depot_tools\NUL GOTO HASDEPOTTOOLS
git clone --depth=1 --branch=master https://chromium.googlesource.com/chromium/tools/depot_tools.git depot_tools
:HASDEPOTTOOLS

IF EXIST src\NUL GOTO HASSRC
cmd /c depot_tools\fetch --nohooks breakpad
GOTO DONESRC
:HASSRC
cmd /c depot_tools\gclient sync --nohooks
:DONESRC

IF EXIST gyp\NUL GOTO HASGYP
git clone --depth=1 --branch=master https://chromium.googlesource.com/external/gyp.git gyp
:HASGYP

IF EXIST build\NUL GOTO HASBUILD
mkdir build
:HASBUILD
cd build

..\gyp\gyp.bat --no-circular-check ..\src\src\client\windows\handler\exception_handler.gyp
msbuild ..\src\src\client\windows\handler\exception_handler.sln /m /p:Configuration=Release

..\gyp\gyp.bat --no-circular-check ..\src\src\client\windows\crash_generation\crash_generation.gyp
msbuild ..\src\src\client\windows\crash_generation\crash_generation.sln /m /p:Configuration=Release

cd ..\..
