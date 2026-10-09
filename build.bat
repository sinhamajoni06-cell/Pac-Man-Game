@echo off
setlocal
if "%~1"=="__run" goto MAIN_START
cmd /c ""%~f0" __run"
set "RC=%ERRORLEVEL%"
powershell -NoProfile -Command "[console]::CursorVisible=$true"
exit /b %RC%

:MAIN_START
cd /d "%~dp0"
title Pacman - Build Tool
powershell -NoProfile -Command "[console]::CursorVisible=$false"

:: ======================================================
:: 1. SETTINGS & PATHS  (edit this section only)
:: ======================================================

set "APP_NAME=Pacman"
set "OUT_DIR=Pacman"
set "MSYS2_ROOT=C:\msys64"

set "COMPILER=g++.exe"
set "CXX_STD=-std=c++17"
set "FLAGS=-Wall -Wextra -O2"
set "DEFINES=-DPACMAN_BUILD"

:: A) Main entry point (the file that contains main())
set "MAIN_FILE=core/src/engine/PacmanMain.cpp"

:: B) Header folders (-I)
set "INCLUDES=-I core/lib/header -I core/src -I core/src/engine -I %MSYS2_ROOT%/ucrt64/include"

:: C) Shared source files (every .cpp in this folder is compiled, not recursive)
set "SOURCES=core/lib/cpp/*.cpp"

:: D) Libraries (-L / -l)
set "LIBS=-L %MSYS2_ROOT%/ucrt64/lib -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system -lopengl32 -lwinmm -lgdi32 -lfreetype"

set "OUTPUT=%OUT_DIR%\%APP_NAME%.exe"
set "MAIN_WIN=%MAIN_FILE:/=\%"

:: ======================================================
:: 2. BANNER
:: ======================================================

echo =========================================
powershell -NoProfile -Command "$w=36; for ($i=0; $i -le $w; $i++) { $p = if ($i -band 1) { 'C' } else { 'c' }; $d = '. ' * [Math]::Max(0, [int](($w - $i) / 2)); Write-Host -NoNewline ([string][char]13 + (' ' * $i)); Write-Host -NoNewline $p -ForegroundColor Yellow; Write-Host -NoNewline (' ' + $d + '        ') -ForegroundColor White; Start-Sleep -Milliseconds 35 }; Write-Host ''"
echo           PACMAN - Build Tool
echo =========================================

:: ======================================================
:: 3. PRE-BUILD CHECKS
:: ======================================================

:: Make MSYS2 UCRT64 tools available even if not in the global PATH
if exist "%MSYS2_ROOT%\ucrt64\bin\g++.exe" set "PATH=%MSYS2_ROOT%\ucrt64\bin;%PATH%"

where %COMPILER% >nul 2>nul
if errorlevel 1 (
    echo [ERROR] %COMPILER% not found. Check your MSYS2 UCRT64 install at %MSYS2_ROOT%.
    pause
    exit /b 1
)

if not exist "%MAIN_WIN%" (
    echo [ERROR] Entry point not found: %MAIN_FILE%
    pause
    exit /b 1
)

dir /b "core\lib\cpp\*.cpp" >nul 2>nul
if errorlevel 1 (
    echo [ERROR] No .cpp files found in core\lib\cpp - add at least one.
    pause
    exit /b 1
)

if not exist "%OUT_DIR%" (
    mkdir "%OUT_DIR%"
    if errorlevel 1 (
        echo [ERROR] Could not create %OUT_DIR% directory.
        pause
        exit /b 1
    )
) else (
    if exist "%OUTPUT%" (
        echo.
        call :ASK "Do you want to Update / Rebuild your project?"
        if errorlevel 2 exit /b 0
    )
)

:: ======================================================
:: 4. BUILD
:: ======================================================

echo.
if exist "%OUTPUT%" del /q "%OUTPUT%"
echo [BUILDING] Compiling %APP_NAME%...
echo Main File : %MAIN_FILE%
echo ----------------------------------------------------

powershell -NoProfile -Command "$src = @(Resolve-Path '%SOURCES%' | ForEach-Object { $_.Path }); $allArgs = @('%CXX_STD%') + '%FLAGS%'.Split(' ') + '%INCLUDES%'.Split(' ') + @('%DEFINES%','%MAIN_FILE%') + $src + '%LIBS%'.Split(' ') + @('-o','%OUTPUT%'); $wd = (Get-Location).Path; $sw = [System.Diagnostics.Stopwatch]::StartNew(); $job = Start-Job -ScriptBlock { param($exe,$jargs,$dir) Set-Location $dir; & $exe @jargs 2>&1 | ForEach-Object { $_.ToString() }; 'EXIT=' + $LASTEXITCODE } -ArgumentList '%COMPILER%',$allArgs,$wd; $pct = 0; while ($job.State -eq 'Running') { if ($pct -lt 90) { $pct += 3 } elseif ($pct -lt 99) { $pct++ }; $filled = [Math]::Floor($pct / 5); $bar = ('#' * $filled) + ('-' * (20 - $filled)); Write-Host -NoNewline ([string][char]13 + '['); Write-Host -NoNewline 'BUILDING' -ForegroundColor Yellow; Write-Host -NoNewline ('] [ ' + $bar + ' ] ' + $pct + '%%   '); Start-Sleep -Milliseconds ([Math]::Max(30, 150 - $pct)) }; $result = @(Receive-Job -Job $job -Wait); Remove-Job -Job $job; $sw.Stop(); $t = $sw.Elapsed.TotalSeconds.ToString('0.00'); $code = $result[-1]; $log = @($result | Select-Object -SkipLast 1); $warn = @($log | Where-Object { $_ -match 'warning:' }).Count; if ($code -eq 'EXIT=0') { Write-Host -NoNewline ([string][char]13 + '['); Write-Host -NoNewline 'FINISHED' -ForegroundColor Green; Write-Host ('] [ #################### ] 100%% - Done in ' + $t + 's, ' + $warn + ' warning(s)      '); if ($warn -gt 0) { $log | ForEach-Object { Write-Host $_ -ForegroundColor DarkYellow } }; exit 0 } else { Write-Host -NoNewline ([string][char]13 + '['); Write-Host -NoNewline 'FAILED' -ForegroundColor Red; Write-Host '] Compilation failed!                                  '; $log | ForEach-Object { Write-Host $_ }; exit 1 }"
if errorlevel 1 goto BUILD_FAILED

:: ======================================================
:: 5. POST-BUILD  (assets + runtime DLLs)
:: ======================================================

echo.
echo =========================================
echo [SUCCESS] Build completed successfully!
echo Output: %OUTPUT%
echo =========================================

if exist "main\assets" (
    echo Syncing assets into %OUT_DIR%\ ...
    robocopy "main\assets" "%OUT_DIR%\main\assets" /E /XO /NFL /NDL /NJH /NJS /NC /NS /NP >nul
    if errorlevel 8 echo [WARNING] Asset sync may have failed.
) else (
    echo [INFO] main\assets not found - skipping asset sync.
)

set "BIN=%MSYS2_ROOT%\ucrt64\bin"
xcopy /y /d "%BIN%\*sfml*.dll"             "%OUT_DIR%\" >nul 2>nul
xcopy /y /d "%BIN%\libfreetype-6.dll"      "%OUT_DIR%\" >nul 2>nul
xcopy /y /d "%BIN%\libgcc_s_seh-1.dll"     "%OUT_DIR%\" >nul 2>nul
xcopy /y /d "%BIN%\libstdc++-6.dll"        "%OUT_DIR%\" >nul 2>nul
xcopy /y /d "%BIN%\libwinpthread-1.dll"    "%OUT_DIR%\" >nul 2>nul
:: Audio dependencies for sfml-audio
xcopy /y /d "%BIN%\*openal*.dll"           "%OUT_DIR%\" >nul 2>nul
xcopy /y /d "%BIN%\libFLAC*.dll"           "%OUT_DIR%\" >nul 2>nul
xcopy /y /d "%BIN%\libvorbis*.dll"         "%OUT_DIR%\" >nul 2>nul
xcopy /y /d "%BIN%\libogg*.dll"            "%OUT_DIR%\" >nul 2>nul

:: ======================================================
:: 6. LAUNCH
:: ======================================================

echo.
call :ASK "Do you want to Launch the game now?"
if errorlevel 2 exit /b 0

echo -----------------------------------------
echo Running %APP_NAME%...
cd /d "%OUT_DIR%"
"%APP_NAME%.exe"
set "GAME_EXIT=%ERRORLEVEL%"
cd /d "%~dp0"
echo.
echo %APP_NAME% exited with code %GAME_EXIT%
pause
exit /b 0

:BUILD_FAILED
echo.
echo =========================================
echo [ERROR] Build Failed! Fix errors above.
echo =========================================
pause
exit /b 1

:: ------------------------------------------------------
:: Subroutine: single-key Y/N prompt
:: Returns errorlevel 1 = Yes, 2 = No
:: ------------------------------------------------------
:ASK
powershell -NoProfile -Command "Write-Host -NoNewline '%~1 ' -ForegroundColor Yellow; Write-Host -NoNewline '[Y/N]: ' -ForegroundColor White; [console]::CursorVisible=$true; while ($true) { $k = [console]::ReadKey($true); if ($k.Key -eq 'Y') { Write-Host 'Y'; [console]::CursorVisible=$false; exit 1 }; if ($k.Key -eq 'N') { Write-Host 'N'; [console]::CursorVisible=$false; exit 2 } }"
exit /b %ERRORLEVEL%
