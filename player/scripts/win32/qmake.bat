@echo off
cd target
if "%1" == "deploy" (
    %QT_STATIC_BIN_PATH%\qmake.exe ..
) else (
    qmake ..
)
