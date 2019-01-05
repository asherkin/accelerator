@echo on

IF EXIST C:\Python27\NUL SET PATH=C:\Python27\;%PATH%

python --version
@IF %errorlevel% neq 0 EXIT /b %errorlevel%

@IF EXIST breakpad\NUL GOTO HASBREAKPAD
mkdir breakpad
:HASBREAKPAD
cd breakpad

@IF EXIST depot_tools\NUL GOTO HASDEPOTTOOLS
git clone --depth=1 --branch=master https://chromium.googlesource.com/chromium/tools/depot_tools.git depot_tools
@IF %errorlevel% neq 0 EXIT /b %errorlevel%
:HASDEPOTTOOLS

@IF EXIST src\NUL GOTO HASSRC
cmd /c depot_tools\fetch --nohooks breakpad
@IF %errorlevel% neq 0 EXIT /b %errorlevel%
GOTO DONESRC
:HASSRC
cmd /c depot_tools\gclient sync --nohooks
@IF %errorlevel% neq 0 EXIT /b %errorlevel%
:DONESRC

@IF EXIST gyp\NUL GOTO HASGYP
git clone --depth=1 --branch=master https://chromium.googlesource.com/external/gyp.git gyp
@IF %errorlevel% neq 0 EXIT /b %errorlevel%
:HASGYP

powershell -Command "& {(Get-Content src\src\build\common.gypi).replace('''WarnAsError'': ''true'',', '''WarnAsError'': ''false'',') | Set-Content src\src\build\common.gypi}"
@IF %errorlevel% neq 0 EXIT /b %errorlevel%

cmd /c gyp\gyp.bat --no-circular-check src\src\client\windows\handler\exception_handler.gyp
@IF %errorlevel% neq 0 EXIT /b %errorlevel%
msbuild src\src\client\windows\handler\exception_handler.sln /m /p:Configuration=Release /p:Platform=Win32
@IF %errorlevel% neq 0 EXIT /b %errorlevel%

cmd /c gyp\gyp.bat --no-circular-check src\src\client\windows\crash_generation\crash_generation.gyp
@IF %errorlevel% neq 0 EXIT /b %errorlevel%
msbuild src\src\client\windows\crash_generation\crash_generation.sln /m /p:Configuration=Release /p:Platform=Win32
@IF %errorlevel% neq 0 EXIT /b %errorlevel%

cmd /c gyp\gyp.bat --no-circular-check src\src\processor\processor.gyp
IF %errorlevel% neq 0 EXIT /b %errorlevel%
@REM msbuild src\src\processor\processor.sln /m /p:Configuration=Release /p:Platform=Win32
@REM IF %errorlevel% neq 0 EXIT /b %errorlevel%
@REM The solution file is currently including a load of broken projects, so just build exactly what we need.
msbuild src\src\processor\processor.vcxproj /m /p:Configuration=Release /p:Platform=Win32
@IF %errorlevel% neq 0 EXIT /b %errorlevel%
msbuild src\src\third_party\libdisasm\libdisasm.vcxproj /m /p:Configuration=Release /p:Platform=Win32
@IF %errorlevel% neq 0 EXIT /b %errorlevel%

cmd /c gyp\gyp.bat --no-circular-check src\src\tools\windows\dump_syms\dump_syms.gyp
@IF %errorlevel% neq 0 EXIT /b %errorlevel%
msbuild src\src\tools\windows\dump_syms\dump_syms.sln /m /p:Configuration=Release /p:Platform=Win32
@IF %errorlevel% neq 0 EXIT /b %errorlevel%

cd ..
