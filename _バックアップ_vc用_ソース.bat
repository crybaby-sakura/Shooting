
@echo off

REM /////////////////////////////////
REM //	backup batch
REM //	ver. 1.04
REM //	11/02/18
REM /////////////////////////////////

set BATCH_TARGET=VC
set BATCH_VERSION=1.04

title backup batch for %BATCH_TARGET% %BATCH_VERSION%

set PROJECT=%~dp0
set PROJECT=%PROJECT:~0,-1%

set COPY_NAME=%PROJECT%_VC_src
set USE_SEVENZIP=1
set SEVENZIP=C:\Program Files\7-Zip\7z.exe
set COMPRESS_LEVEL=9
set EXCLUDE=%~n0.txt

REM ///////////////////////////////// フォルダチェック

cd ..

set DATE_TMP=%date%
set DATE_NOW=%DATE_TMP:~-8,2%%DATE_TMP:~-5,2%%DATE_TMP:~-2,2%

set NUM=0

:loop

set /a NUM = NUM + 1

set TARGET=%COPY_NAME%_%DATE_NOW%_%NUM%
if not exist %TARGET% goto backup

goto loop

REM ///////////////////////////////// コピー

:backup

set EXCLUDE_FILE=%~dp0%EXCLUDE%

if not exist "%EXCLUDE_FILE%" goto err

echo %TARGET%
mkdir %TARGET%
xcopy /Y /E /D %PROJECT% %TARGET% /EXCLUDE:%EXCLUDE_FILE%

REM ///////////////////////////////// 圧縮

if "%USE_SEVENZIP%" == "1" (
	echo sevenzip compress...
	echo %TARGET%.7z

	if not exist "%SEVENZIP%" goto err

	if exist "%TARGET%.7z" del "%TARGET%.7z"

	"%SEVENZIP%" u -t7z -mx=%COMPRESS_LEVEL% "%TARGET%.7z" "%TARGET%" > nul
)

echo 正常終了
pause

goto :EOF

REM ///////////////////////////////// エラーならポーズ

:err

echo エラー
pause

goto :EOF
