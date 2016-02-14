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

cd ..
