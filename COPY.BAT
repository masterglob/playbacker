@echo off
@set PUTTYPATH="C:\Program Files\PuTTY"
@set USER=tc
@set IP=192.168.7.80
@set PPK=C:\Users\MAO\Documents\_ssh\id-rsa-priv.ppk
@set TARGET_PATH=/tmp/_dev

cd C:\Users\MAO\Documents\Arduino\Rasberry\PROJECTS\playbacker
@echo Copying source files to target


::@%PUTTYPATH%\pscp
%PUTTYPATH%\pscp -i %PPK% -r _dev/ %USER%@%IP%:%TARGET_PATH%
