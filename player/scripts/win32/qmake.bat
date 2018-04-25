@echo off
cd target
if "%1" == "deploy" (
    %QMAKE_STATIC% ..
) else (
    qmake ..
)
