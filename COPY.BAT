cd "%~dp0"

@set IP=192.168.7.80
@set USER=tc
@set PPK=.ssh\id_rsa.ppk

@echo off
@set PUTTYPATH="C:\Program Files\PuTTY"
@set TARGET_PATH=/tmp/_dev

@echo Copying source files to target


::@%PUTTYPATH%\pscp
%PUTTYPATH%\pscp -i %PPK% -r _dev/ %USER%@%IP%:%TARGET_PATH%
