@echo off
setlocal enabledelayedexpansion

echo -------------------------
echo ^| Key     ^| Value
echo -------------------------
echo ^| A       ^| 0
echo ^| B       ^| 1
echo ^| Select  ^| 2
echo ^| Start   ^| 3
echo ^| Right   ^| 4
echo ^| Left    ^| 5
echo ^| Up      ^| 6
echo ^| Down    ^| 7
echo ^| R       ^| 8
echo ^| L       ^| 9
echo ^| X       ^| 10
echo ^| Y       ^| 11
echo ^| Hinge   ^| 12
echo ^| Debug   ^| 13
echo -------------------------

echo Please enter between 3 and 14 numbers (0-13), separated by spaces:
set /p input=

rem Split input into tokens and validate count
set count=0
for %%a in (%input%) do (
    set /a count+=1
)
if %count% lss 3 (
    echo ERROR: Please enter at least 3 values.
    exit /b 1
)
if %count% gtr 14 (
    echo ERROR: Please enter no more than 14 values.
    exit /b 1
)

set result=0

set power0=1
set power1=2
set power2=4
set power3=8
set power4=16
set power5=32
set power6=64
set power7=128
set power8=256
set power9=512
set power10=1024
set power11=2048
set power12=4096
set power13=8192

for %%a in (%input%) do (
    set val=%%a
    if !val! lss 0 (
        echo ERROR: Value !val! less than 0
        exit /b 1
    )
    if !val! gtr 13 (
        echo ERROR: Value !val! greater than 13
        exit /b 1
    )
    call set "pow=%%power!val!%%"
    set /a result=result ^| pow
)

for /f %%h in ('powershell -NoProfile -Command "('{0:X}' -f %result%)"') do set hex=%%h

echo Your converted hotkey value is %hex%. Please place this in ndsbs.ini in the hotkey value.
pause
