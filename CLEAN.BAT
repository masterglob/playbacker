@echo off
@set PUTTYPATH="C:\Program Files\PuTTY"
@set USER=tc
@set IP=192.168.7.80
@set PPK=C:\Users\MAO\Documents\_ssh\id-rsa-priv.ppk
@set TARGET_PATH=/home/%USER%/_dev

cd C:\Users\MAO\Documents\Arduino\Rasberry\PROJECTS\playbacker


::@%PUTTYPATH%\pscp
%PUTTYPATH%\pscp -i %PPK% -r src/ %USER%@%IP%:/home/tc/_dev/src

@echo Compiling
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "make -C %TARGET_PATH% clean all"