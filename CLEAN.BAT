@set IP=192.168.1.6
@set USER=tc
@set PPK=C:\Users\MAO\Documents\_ssh\id-rsa-priv.ppk

@echo off
@set PUTTYPATH="C:\Program Files\PuTTY"
@set TARGET_PATH=/home/%USER%/_dev

cd C:\Users\MAO\Documents\Arduino\Rasberry\PROJECTS\playbacker


::@%PUTTYPATH%\pscp
%PUTTYPATH%\pscp -i %PPK% -r src/ %USER%@%IP%:/home/tc/_dev/src

@echo Compiling
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "make -C %TARGET_PATH% clean all"