REM Copy packages data
xcopy twitchd\target\release\twitchd.exe        dist\packages\twitchd\data\         /y
xcopy player\target\release\twitch-player.exe   dist\packages\twitch_player\data\   /y
xcopy %LIBVLC_DIST%\*                           dist\packages\libvlc\data\          /y /e

cd dist

For /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set date=%%c-%%a-%%b)
For /f "tokens=1-3 delims=/:" %%a in ("%TIME%") do (set time=%%a%%b%%c)
set stamp=%date%_%time%
echo %stamp%
%QT_STATIC_BIN_PATH%\repogen.exe -p packages "twitch-player_%stamp%"
%QT_STATIC_BIN_PATH%\binarycreator.exe ^
    -t %QT_STATIC_BIN_PATH%\installerbase.exe ^
    -p packages ^
    --online-only ^
    -c config\config.xml ^
    twitch-player_win32_online.exe
