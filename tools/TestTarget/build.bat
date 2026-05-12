@echo off
REM ====================================================================
REM   build.bat - compile TestTarget.exe
REM
REM   YEU CAU: chay trong "Developer Command Prompt for VS 2022"
REM   (Start Menu -> Visual Studio 2022 -> Developer Command Prompt)
REM
REM   Ket qua: TestTarget.exe trong folder hien tai.
REM ====================================================================
echo Compiling TestTarget.cpp...
cl /nologo /EHsc /W3 /O2 /MD /utf-8 TestTarget.cpp /link /SUBSYSTEM:CONSOLE
if errorlevel 1 (
    echo.
    echo BUILD FAILED. Hay chac chan ban dang chay trong Developer Command Prompt.
    pause
    exit /b 1
)
echo.
echo OK! Da tao TestTarget.exe
echo.
echo Chay: TestTarget.exe
pause
