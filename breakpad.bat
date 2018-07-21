@echo on

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

powershell -Command "& {(Get-Content src\src\build\common.gypi).replace('''WarnAsError'': ''true'',', '''WarnAsError'': ''false'',') | Set-Content src\src\build\common.gypi}"

cmd /c gyp\gyp.bat --no-circular-check src\src\client\windows\handler\exception_handler.gyp
msbuild src\src\client\windows\handler\exception_handler.sln /m /p:Configuration=Release

cmd /c gyp\gyp.bat --no-circular-check src\src\client\windows\crash_generation\crash_generation.gyp
msbuild src\src\client\windows\crash_generation\crash_generation.sln /m /p:Configuration=Release

cmd /c gyp\gyp.bat --no-circular-check src\src\processor\processor.gyp
msbuild src\src\processor\processor.sln /m /p:Configuration=Release

cmd /c gyp\gyp.bat --no-circular-check src\src\tools\windows\dump_syms\dump_syms.gyp
msbuild src\src\tools\windows\dump_syms\dump_syms.sln /m /p:Configuration=Release

cd ..
